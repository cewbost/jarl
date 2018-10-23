#include "function.h"

#include "code_generator.h"

#ifndef NDEBUG
#include <cstdio>
//#define PRINT_CODE
#else
#undef PRINT_CODE
#endif

using namespace CodeGenerator;

constexpr OpCodeType stack_pos_bits     = 0xe000;
constexpr OpCodeType stack_pos_arg      = 0x0000;
constexpr OpCodeType stack_pos_local    = 0x8000;
constexpr OpCodeType stack_pos_const    = 0x4000;
constexpr OpCodeType stack_pos_capture  = 0x2000;

struct ThreadingContext {
  std::vector<OpCodeType> code;
  std::vector<std::pair<int, int>> code_positions;
  
  VarAllocMap *var_allocs, *context_var_allocs;
  VectorSet<TypedValue> constants;
  
  int stack_size;
  unsigned short arguments, captures;
  
  std::vector<std::unique_ptr<char[]>> *errors;
  
  ThreadingContext(
    decltype(errors) err,
    decltype(var_allocs) va = nullptr,
    decltype(context_var_allocs) cva = nullptr,
    unsigned short args = 0,
    unsigned short caps = 0
  )
  : var_allocs(std::move(va)),
    context_var_allocs(cva),
    errors(err),
    stack_size(0),
    arguments(args),
    captures(caps) {}
  
  void putInstruction(OpCodeType op, int pos);
  
  void threadAST(ASTNode*, ASTNode* = nullptr);
  void threadRexpr(ASTNode* node, ASTNode* prev_node);
  void threadInsRexpr(ASTNode* node, ASTNode* prev_node);
  
private:
  class AllocContextGuard {
    VarAllocMap*& alloc_map_;
  public:
    AllocContextGuard(VarAllocMap*& alloc_map): alloc_map_(alloc_map){
      alloc_map_ = new VarAllocMap(alloc_map_);
    }
    ~AllocContextGuard(){
      VarAllocMap* old_allocs = alloc_map_;
      alloc_map_ = alloc_map_->get_delegate();
      delete old_allocs;
    }
  };
  
public:
  AllocContextGuard newScope(){
    return AllocContextGuard(this->var_allocs);
  }
};

namespace {
  
  Function* generate_(
    std::unique_ptr<ASTNode>&& parse_tree,
    std::vector<std::unique_ptr<char[]>>* errors,
    VarAllocMap* var_allocs,
    VarAllocMap* context_var_allocs = nullptr
  ){
    assert(var_allocs != nullptr);
    
    ThreadingContext context(
      errors,
      std::move(var_allocs),
      context_var_allocs,
      var_allocs->size()
    );
    
    int prev_errors = errors->size();
    auto pos = parse_tree->pos.first;
    context.code_positions.emplace_back(pos, 0);
    context.threadAST(parse_tree.get());
    if(errors->size() > prev_errors) return nullptr;
    context.putInstruction(Op::Return, pos);
    
    //correct stack positions
    int extended = 0;
    const int capture_pos = context.arguments;
    const int const_pos   = capture_pos + context.captures;
    const int local_pos   = const_pos + context.constants.size();
    for(auto& op: context.code){
      if(extended == 0){
        if(op & Op::Extended){
          if(op & Op::Int) extended = 1;
          else extended = 2;
        }
      }else if(extended == 1){
        extended = 0;
      }else{
        extended = 0;
        switch(op & stack_pos_bits){
        case stack_pos_capture:
          op = (op & ~stack_pos_capture) + capture_pos;
          break;
        case stack_pos_const:
          op = (op & ~stack_pos_const) + const_pos;
          break;
        case stack_pos_local:
          op = (op & ~stack_pos_local) + local_pos;
          break;
        }
      }
    }
    
    auto func = new Function(
      std::move(context.code),
      std::move(context.constants),
      std::move(context.code_positions),
      context.arguments,
      context.captures
    );
    
    #ifdef PRINT_CODE
    fprintf(stderr, "::proc %p::\n", func);
    fprintf(stderr, "%s\n", func->opcodesToStrDebug().c_str());
    #endif
    
    return func;
  }
}

void ThreadingContext::putInstruction(OpCodeType op, int pos){
  if(this->code_positions.back().first != pos){
    this->code_positions.emplace_back(pos, this->code.size());
  }
  this->code.push_back(op);
}

//Threading functions
#define D_putInstruction(ins) this->putInstruction(ins, node->pos.first)
#define D_breakError(msg, node, ...) \
  errors->emplace_back(dynSprintf( \
    "line %d: " msg, \
    node ->pos.first \
  )); \
  D_putInstruction(Op::Nop); \
  break;
#define D_breakErrorVargs(msg, node, ...) \
  errors->emplace_back(dynSprintf( \
    "line %d: " msg, \
    node ->pos.first, \
    __VA_ARGS__ \
  )); \
  D_putInstruction(Op::Nop); \
  break;

void ThreadingContext::threadAST(ASTNode* node, ASTNode* prev_node){
  
  switch(static_cast<ASTNodeType>(static_cast<unsigned>(node->type) & ~0xff)){
  case ASTNodeType::Error:
    assert(false);
    break;
    
  case ASTNodeType::Value:
    switch(node->type){
    case ASTNodeType::Bool:
      if(node->bool_value){
        D_putInstruction(Op::PushTrue);
      }else{
        D_putInstruction(Op::PushFalse);
      }
      break;
    case ASTNodeType::Int:
      if(node->int_value <= std::numeric_limits<OpCodeSignedType>::max()
      && node->int_value >= std::numeric_limits<OpCodeSignedType>::min()){
        D_putInstruction(Op::Push | Op::Extended | Op::Int);
        D_putInstruction(static_cast<OpCodeType>(node->int_value));
      }else{
        D_putInstruction(Op::Push | Op::Extended);
        D_putInstruction(
          (OpCodeType)(constants.emplace(node->int_value) | stack_pos_const)
        );
      }
      break;
    case ASTNodeType::Float:
      D_putInstruction(Op::Push | Op::Extended);
      D_putInstruction(
        (OpCodeType)(constants.emplace(node->float_value) | stack_pos_const)
      );
      break;
    case ASTNodeType::String:
      D_putInstruction(Op::Push | Op::Extended);
      D_putInstruction(
        (OpCodeType)(constants.emplace(node->string_value) | stack_pos_const)
      );
      break;
    case ASTNodeType::Identifier:
      {
        D_putInstruction(Op::Push | Op::Extended);
        
        auto it = var_allocs->find(node->string_value);
        if(it == var_allocs->end()){
          if(context_var_allocs == nullptr){
            D_breakErrorVargs("Undeclared identifier '%s'", node,
              node->string_value->str()
            );
          }
          
          it = context_var_allocs->find(node->string_value);
          
          if(it == context_var_allocs->end()){
            D_breakErrorVargs("Undeclared identifier '%s'", node,
              node->string_value->str()
            );
          }
          
          auto pos = (captures++) | stack_pos_capture;
          var_allocs->base()[node->string_value] = pos;
          D_putInstruction(pos);
        }else{
          D_putInstruction(it->second);
        }
      }
      break;
    case ASTNodeType::Null:
      D_putInstruction(Op::Push);
      break;
    default:
      assert(false);
    }
    ++stack_size;
    break;
    
  case ASTNodeType::UnaryExpr:
    threadAST(node->child, node);
    switch(node->type){
    case ASTNodeType::Neg:
      D_putInstruction(Op::Neg);
      break;
    case ASTNodeType::Not:
      D_putInstruction(Op::Not);
      break;
    default:
      assert(false);
    }
    break;
    
  case ASTNodeType::BinaryExpr:
    threadAST(node->children.first, node);
    threadAST(node->children.second, node);
    switch(node->type){
    case ASTNodeType::Apply:
      D_putInstruction(Op::Apply);
      break;
      
    case ASTNodeType::Add:
      D_putInstruction(Op::Add);
      break;
    case ASTNodeType::Sub:
      D_putInstruction(Op::Sub);
      break;
    case ASTNodeType::Mul:
      D_putInstruction(Op::Mul);
      break;
    case ASTNodeType::Div:
      D_putInstruction(Op::Div);
      break;
    case ASTNodeType::Mod:
      D_putInstruction(Op::Mod);
      break;
    case ASTNodeType::Append:
      D_putInstruction(Op::Append);
      break;
      
    case ASTNodeType::Cmp:
      D_putInstruction(Op::Cmp);
      break;
    case ASTNodeType::Eq:
      D_putInstruction(Op::Eq);
      break;
    case ASTNodeType::Neq:
      D_putInstruction(Op::Neq);
      break;
    case ASTNodeType::Gt:
      D_putInstruction(Op::Gt);
      break;
    case ASTNodeType::Lt:
      D_putInstruction(Op::Lt);
      break;
    case ASTNodeType::Geq:
      D_putInstruction(Op::Geq);
      break;
    case ASTNodeType::Leq:
      D_putInstruction(Op::Leq);
      break;
    
    case ASTNodeType::In:
      D_putInstruction(Op::In);
      break;
    
    default:
      assert(false);
    }
    --stack_size;
    break;
    
  case ASTNodeType::BranchExpr:
    {
      unsigned jmp_addr;
      threadAST(node->children.first, node);
      
      switch(node->type){
      case ASTNodeType::And:
        D_putInstruction(Op::Jfsc | Op::Extended);
        jmp_addr = code.size();
        D_putInstruction((OpCodeType)0);
        --stack_size;
        threadAST(node->children.second, node);
        code[jmp_addr] = (OpCodeType)code.size();
        break;
        
      case ASTNodeType::Or:
        D_putInstruction(Op::Jtsc | Op::Extended);
        jmp_addr = code.size();
        D_putInstruction((OpCodeType)0);
        --stack_size;
        threadAST(node->children.second, node);
        code[jmp_addr] = (OpCodeType)code.size();
        break;
      
      case ASTNodeType::If:
        
        D_putInstruction(Op::Jf | Op::Extended);
        jmp_addr = code.size();
        D_putInstruction((OpCodeType)0);
        --stack_size;
        
        if(node->children.second->type == ASTNodeType::Else){
          threadAST(node->children.second->children.first, node);
          --stack_size;
          D_putInstruction(Op::Jmp | Op::Extended);
          code[jmp_addr] = (OpCodeType)code.size() + 1;
          jmp_addr = code.size();
          D_putInstruction((OpCodeType)0);
          threadAST(node->children.second->children.second, node);
          code[jmp_addr] = (OpCodeType)code.size();
        }else{
          threadAST(node->children.second, node);
          --stack_size;
          D_putInstruction(Op::Jmp | Op::Extended);
          code[jmp_addr] = (OpCodeType)code.size() + 1;
          jmp_addr = code.size();
          D_putInstruction((OpCodeType)0);
          D_putInstruction(Op::Push);
          ++stack_size;
          code[jmp_addr] = (OpCodeType)code.size();
        }
        break;
        
      default:
        assert(false);
      }
    }
    break;
    
  case ASTNodeType::AssignExpr:
    {
      if(node->type == ASTNodeType::Insert){
        threadInsRexpr(node->children.first, node);
      }else{
        threadRexpr(node->children.first, node);
      }
      threadAST(node->children.second, node);
      
      switch(node->type){
      case ASTNodeType::Assign:
      case ASTNodeType::Insert:
        D_putInstruction(Op::Write | Op::Borrowed);
        break;
      case ASTNodeType::AddAssign:
        D_putInstruction(Op::Add | Op::Borrowed);
        break;
      case ASTNodeType::SubAssign:
        D_putInstruction(Op::Sub | Op::Borrowed);
        break;
      case ASTNodeType::MulAssign:
        D_putInstruction(Op::Mul | Op::Borrowed);
        break;
      case ASTNodeType::DivAssign:
        D_putInstruction(Op::Div | Op::Borrowed);
        break;
      case ASTNodeType::ModAssign:
        D_putInstruction(Op::Mod | Op::Borrowed);
        break;
      case ASTNodeType::AppendAssign:
        D_putInstruction(Op::Append | Op::Borrowed);
        break;
      default:
        assert(false);
      }
      --stack_size;
    }
    break;
    
  default:
    switch(node->type){
    case ASTNodeType::CodeBlock:
      {
        auto guard = this->newScope();
        threadAST(node->children.first, node);
        
        auto size = var_allocs->direct().size();
        if(size > 0){
          if(size == 1){
            D_putInstruction(Op::Reduce);
          }else{
            D_putInstruction(Op::Reduce | Op::Extended);
            D_putInstruction(static_cast<OpCodeType>(size));
          }
          stack_size -= size;
        }
      }
      break;
      
    case ASTNodeType::Array:
      if(node->child->type == ASTNodeType::Nop){
        D_putInstruction(Op::CreateArray);
        ++stack_size;
      }else{
        OpCodeType elems = 0;
        for(auto it = node->child->exprListIterator(); it != nullptr; ++it){
          if(it->type == ASTNodeType::Nop) continue;
          threadAST(it.get(), node);
          ++elems;
        }
        D_putInstruction(Op::CreateArray | Op::Extended | Op::Int);
        D_putInstruction(elems);
        stack_size -= elems - 1;
      }
      break;
    
    case ASTNodeType::Table:
      if(node->child->type == ASTNodeType::Nop){
        D_putInstruction(Op::CreateTable);
        ++stack_size;
      }else{
        OpCodeType elems = 0;
        for(auto it = node->child->exprListIterator(); it != nullptr; ++it){
          if(it->type == ASTNodeType::KeyValuePair){
            threadAST(it->children.first, node);
            threadAST(it->children.second, node);
            ++elems;
          }else if(it->type == ASTNodeType::Nop){
            continue;
          }else assert(false);
        }
        D_putInstruction(Op::CreateTable | Op::Extended | Op::Int);
        D_putInstruction(elems);
        stack_size -= 2 * elems - 1;
      }
      break;
      
    case ASTNodeType::While:
      {
        unsigned begin_addr = code.size();
        
        threadAST(node->children.first, node);
        
        D_putInstruction(Op::Jf | Op::Extended);
        --stack_size;
        unsigned end_jmp_addr = code.size();
        D_putInstruction((OpCodeType)0);
        
        threadAST(node->children.second, node);
        D_putInstruction(Op::Pop);
        --stack_size;
        D_putInstruction(Op::Jmp | Op::Extended);
        D_putInstruction((OpCodeType)begin_addr);
        code[end_jmp_addr] = (OpCodeType)code.size();
        
        D_putInstruction(Op::Push);
        ++stack_size;
      }
      break;
    
    case ASTNodeType::For:
      {
        auto guard = this->newScope();
        {
          ASTNode* stepper = node->children.first;
          if(stepper->type == ASTNodeType::ExprList){
            var_allocs->direct()[stepper->children.first->string_value] =
              stack_size + 1 | stack_pos_local;
            stepper = stepper->children.second;
          }
          var_allocs->direct()[stepper->children.first->string_value] =
            stack_size + 2 | stack_pos_local;
          
          threadAST(stepper->children.second, node);
          D_putInstruction(Op::BeginIter);
          stack_size += 2;
        }
        
        unsigned begin_addr = code.size();
        D_putInstruction(Op::NextOrJmp | Op::Extended);
        unsigned end_jmp_addr = code.size();
        D_putInstruction((OpCodeType)0);
        
        threadAST(node->children.second, node);
        
        D_putInstruction(Op::Pop);
        --stack_size;
        D_putInstruction(Op::Jmp | Op::Extended);
        D_putInstruction((OpCodeType)begin_addr);
        code[end_jmp_addr] = (OpCodeType)code.size();
        D_putInstruction(Op::Push);
        stack_size -= 2;
      }
      break;
      
    case ASTNodeType::Function:
      {
        auto var_alloc = new VarAllocMap(nullptr);
        OpCodeType args = 0;
        for(auto it = node->children.first->exprListIterator(); it != nullptr; ++it){
          (*var_alloc)[it->string_value] = args++;
        }
        
        auto func = generate_(
          std::unique_ptr<ASTNode>(node->children.second),
          errors, var_alloc, var_allocs
        );
        if(func == nullptr){
          D_putInstruction(Op::Nop);
          break;
        }
        
        node->children.second = nullptr;
        D_putInstruction(Op::Push | Op::Extended);
        D_putInstruction((OpCodeType)constants.emplace(func) | stack_pos_const);
        ++stack_size;
        
        if(func->captures > 0){
          auto& base = static_cast<VectorMapBase&>(var_alloc->base());
          for(int i = 0; i < func->captures; ++i){
            D_putInstruction(Op::Push | Op::Extended);
            D_putInstruction((*var_allocs)[base[i + args].first]);
          }
          D_putInstruction(Op::CreateClosure | Op::Extended | Op::Int);
          D_putInstruction((OpCodeType)func->captures);
        }
        
        delete var_alloc;
      }
      break;
      
    case ASTNodeType::Seq:
      threadAST(node->children.first, node);
      D_putInstruction(Op::Pop);
      --stack_size;
      threadAST(node->children.second, node);
      break;
    
    case ASTNodeType::Var:
      {
        auto subnode = node->child;
        
        auto old_stack_size = stack_size;
        threadAST(subnode->children.second, subnode);
        auto it = var_allocs->direct()
          .find(subnode->children.first->string_value);
        if(it != var_allocs->direct().end()){
          D_putInstruction(Op::Write | Op::Extended | Op::Dest);
          D_putInstruction(it->second);
        }else{
          var_allocs->direct()[subnode->children.first->string_value] =
            old_stack_size | stack_pos_local;
        }
        D_putInstruction(Op::Push);
        ++stack_size;
      }
      break;
    
    case ASTNodeType::Return:
      threadAST(node->child, node);
      D_putInstruction(Op::Return);
      break;
    
    case ASTNodeType::Recurse:
      D_putInstruction(Op::Push);
      if(node->child->type == ASTNodeType::Nop){
        D_putInstruction(Op::Recurse);
      }else{
        OpCodeType elems = 0;
        for(auto it = node->child->exprListIterator(); it != nullptr; ++it){
          if(it->type == ASTNodeType::Nop) continue;
          threadAST(it.get(), node);
          ++elems;
        }
        D_putInstruction(Op::Recurse | Op::Extended);
        D_putInstruction(elems);
        stack_size -= elems;
      }
      break;
    
    case ASTNodeType::Print:
      {
        int num = 0;
        for(auto it = node->child->exprListIterator(); it != nullptr; ++it){
          threadAST(*it, node);
          ++num;
        }
        if(num == 1){
          D_putInstruction(Op::Print);
        }else if(num > 1){
          D_putInstruction(Op::Print | Op::Extended | Op::Int);
          D_putInstruction((OpCodeType)num);
        }else /*nop*/;
        D_putInstruction(Op::Push);
      }
      break;
      
    case ASTNodeType::Assert:
      if(node->child->type == ASTNodeType::ExprList){
        threadAST(node->child->children.second, node);
        threadAST(node->child->children.first, node);
        D_putInstruction(Op::Assert | Op::Alt1);
        --stack_size;
      }else{
        threadAST(node->child, node);
        D_putInstruction(Op::Assert);
      }
      break;
      
    case ASTNodeType::Index:
      {
        threadAST(node->children.first, node);
        
        if(node->children.second->type == ASTNodeType::ExprList){
          threadAST(node->children.second->children.first, node);
          threadAST(node->children.second->children.second, node);
          D_putInstruction(Op::Slice);
          stack_size -= 2;
        }else{
          threadAST(node->children.second, node);
          D_putInstruction(Op::Get);
          --stack_size;
        }
      }
      break;
      
    case ASTNodeType::Call:
      threadAST(node->children.first, node);
      if(node->children.second->type == ASTNodeType::Nop){
        D_putInstruction(Op::Call);
      }else{
        OpCodeType elems = 0;
        for(auto it = node->children.second->exprListIterator(); it != nullptr; ++it){
          if(it->type == ASTNodeType::Nop) continue;
          threadAST(it.get(), node);
          ++elems;
        }
        D_putInstruction(Op::Call | Op::Extended);
        D_putInstruction(elems);
        stack_size -= elems;
      }
      break;
      
    case ASTNodeType::Nop:
      D_putInstruction(Op::Push);
      ++stack_size;
      break;
      
    default:
      assert(false);
    }
  }
}

void ThreadingContext::threadRexpr(ASTNode* node, ASTNode* prev_node){
  switch(node->type){
  case ASTNodeType::Identifier:
    if(
      auto it = var_allocs->find(node->string_value);
      it == var_allocs->end()
    ){
      D_breakErrorVargs("Undeclared identifier '%s'", node,
        node->string_value->str()
      );
    }else{
      D_putInstruction(Op::Borrow | Op::Extended);
      D_putInstruction(it->second);
      ++stack_size;
    }
    break;
  case ASTNodeType::Index:
    this->threadRexpr(node->children.first, node);
    this->threadAST(node->children.second, node);
    D_putInstruction(Op::Borrow | Op::Borrowed);
    --stack_size;
    break;
  }
}

void ThreadingContext::threadInsRexpr(ASTNode* node, ASTNode* prev_node){
  if(node->type == ASTNodeType::Index){
    this->threadRexpr(node->children.first, node);
    this->threadAST(node->children.second, node);
    D_putInstruction(Op::Borrow | Op::Borrowed | Op::Alt1);
    --stack_size;
  }else{
    if(node->type == ASTNodeType::Identifier){
      errors->emplace_back(dynSprintf(
        "line %d: Insertion to variable", node->pos.first
      ));
    }
    D_putInstruction(Op::Nop);
  }
}

#undef D_putInstruction
#undef D_breakError
#undef D_breakErrorVargs

namespace CodeGenerator {
  
  Function* generate(
    std::unique_ptr<ASTNode>&& parse_tree,
    std::vector<std::unique_ptr<char[]>>* errors
  ){
    VarAllocMap* var_allocs = new VarAllocMap(nullptr);
    auto ret = generate_(std::move(parse_tree), errors, var_allocs);
    delete var_allocs;
    return ret;
  }
}
