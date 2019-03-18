#include "function.h"

#include "code_generator.h"

#ifndef NDEBUG
#include <cstdio>
//#define PRINT_CODE
#else
#undef PRINT_CODE
#endif

using namespace CodeGenerator;

constexpr OpCodes::Type stack_pos_bits     = 0xe000;
constexpr OpCodes::Type stack_pos_arg      = 0x0000;
constexpr OpCodes::Type stack_pos_local    = 0x8000;
constexpr OpCodes::Type stack_pos_const    = 0x4000;
constexpr OpCodes::Type stack_pos_capture  = 0x2000;

struct CodeGenerationContext {
  std::vector<OpCodes::Type> code;
  std::vector<std::pair<int, int>> code_positions;
  
  VarAllocMap *var_allocs, *context_var_allocs;
  VectorSet<TypedValue> constants;
  
  unsigned arguments, captures, locals;
  
  std::vector<std::unique_ptr<char[]>> *errors;
  
  CodeGenerationContext(
    decltype(errors) err,
    decltype(var_allocs) va = nullptr,
    decltype(context_var_allocs) cva = nullptr,
    unsigned args = 0,
    unsigned caps = 0,
    unsigned locs = 0
  )
  : var_allocs(std::move(va)),
    context_var_allocs(cva),
    errors(err),
    arguments(args),
    captures(caps),
    locals(locs){}
  
  void putInstruction(OpCodes::Type op, int pos);
  
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
    
    CodeGenerationContext context(
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
    if(!parse_tree->isValue()){
      context.putInstruction(OpCodes::Push, pos);
    }
    context.putInstruction(OpCodes::Return, pos);
    
    //correct stack positions
    int extended = 0;
    const int capture_pos = context.arguments;
    const int const_pos   = capture_pos + context.captures;
    const int local_pos   = const_pos + context.constants.size();
    for(auto& op: context.code){
      if(extended == 0){
        if(op & OpCodes::Extended){
          if(op & OpCodes::Int) extended = 1;
          else if(op & OpCodes::Extended2) extended = 4;
          else extended = 2;
        }
      }else if(extended == 1){
        extended = 0;
      }else{
        extended -= 2;
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
      context.captures,
      context.locals
    );
    
    #ifdef PRINT_CODE
    fprintf(stderr, "::proc %p::\n", func);
    fprintf(stderr, "%s\n", func->opcodesToStrDebug().c_str());
    #endif
    
    return func;
  }
}

void CodeGenerationContext::putInstruction(OpCodes::Type op, int pos){
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
  D_putInstruction(OpCodes::Nop); \
  break;
#define D_breakErrorVargs(msg, node, ...) \
  errors->emplace_back(dynSprintf( \
    "line %d: " msg, \
    node ->pos.first, \
    __VA_ARGS__ \
  )); \
  D_putInstruction(OpCodes::Nop); \
  break;

void CodeGenerationContext::threadAST(ASTNode* node, ASTNode* prev_node){
  
  switch(static_cast<ASTNodeType>(static_cast<unsigned>(node->type) & ~0xff)){
  case ASTNodeType::Error:
    assert(false);
    break;
    
  case ASTNodeType::Value:
    switch(node->type){
    case ASTNodeType::Bool:
      if(node->bool_value){
        D_putInstruction(OpCodes::PushTrue);
      }else{
        D_putInstruction(OpCodes::PushFalse);
      }
      break;
    case ASTNodeType::Int:
      if(node->int_value <= std::numeric_limits<OpCodes::SignedType>::max()
      && node->int_value >= std::numeric_limits<OpCodes::SignedType>::min()){
        D_putInstruction(OpCodes::Push | OpCodes::Extended | OpCodes::Int);
        D_putInstruction(static_cast<OpCodes::Type>(node->int_value));
      }else{
        D_putInstruction(OpCodes::Push | OpCodes::Extended);
        D_putInstruction(
          (OpCodes::Type)(constants.emplace(node->int_value) | stack_pos_const)
        );
      }
      break;
    case ASTNodeType::Float:
      D_putInstruction(OpCodes::Push | OpCodes::Extended);
      D_putInstruction(
        (OpCodes::Type)(constants.emplace(node->float_value) | stack_pos_const)
      );
      break;
    case ASTNodeType::String:
      D_putInstruction(OpCodes::Push | OpCodes::Extended);
      D_putInstruction(
        (OpCodes::Type)(constants.emplace(node->string_value) | stack_pos_const)
      );
      break;
    case ASTNodeType::Identifier:
      {
        D_putInstruction(OpCodes::Push | OpCodes::Extended);
        
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
      D_putInstruction(OpCodes::Push);
      break;
    default:
      assert(false);
    }
    break;
    
  case ASTNodeType::UnaryExpr:
    if(node->type == ASTNodeType::Move){
      threadRexpr(node->child, node);
      D_putInstruction(OpCodes::Move | OpCodes::Borrowed);
    }else{
      threadAST(node->child, node);
      switch(node->type){
      case ASTNodeType::Neg:
        D_putInstruction(OpCodes::Neg);
        break;
      case ASTNodeType::Not:
        D_putInstruction(OpCodes::Not);
        break;
      case ASTNodeType::Move:
        
        D_putInstruction(OpCodes::Move);
        break;
      default:
        assert(false);
      }
    }
    break;
    
  case ASTNodeType::BinaryExpr:
    threadAST(node->children.first, node);
    threadAST(node->children.second, node);
    switch(node->type){
    case ASTNodeType::Apply:
      D_putInstruction(OpCodes::Apply);
      break;
      
    case ASTNodeType::Add:
      D_putInstruction(OpCodes::Add);
      break;
    case ASTNodeType::Sub:
      D_putInstruction(OpCodes::Sub);
      break;
    case ASTNodeType::Mul:
      D_putInstruction(OpCodes::Mul);
      break;
    case ASTNodeType::Div:
      D_putInstruction(OpCodes::Div);
      break;
    case ASTNodeType::Mod:
      D_putInstruction(OpCodes::Mod);
      break;
    case ASTNodeType::Append:
      D_putInstruction(OpCodes::Append);
      break;
      
    case ASTNodeType::Cmp:
      D_putInstruction(OpCodes::Cmp);
      break;
    case ASTNodeType::Eq:
      D_putInstruction(OpCodes::Eq);
      break;
    case ASTNodeType::Neq:
      D_putInstruction(OpCodes::Neq);
      break;
    case ASTNodeType::Gt:
      D_putInstruction(OpCodes::Gt);
      break;
    case ASTNodeType::Lt:
      D_putInstruction(OpCodes::Lt);
      break;
    case ASTNodeType::Geq:
      D_putInstruction(OpCodes::Geq);
      break;
    case ASTNodeType::Leq:
      D_putInstruction(OpCodes::Leq);
      break;
    
    case ASTNodeType::In:
      D_putInstruction(OpCodes::In);
      break;
    
    default:
      assert(false);
    }
    break;
    
  case ASTNodeType::BranchExpr:
    {
      unsigned jmp_addr;
      threadAST(node->children.first, node);
      
      switch(node->type){
      case ASTNodeType::And:
        D_putInstruction(OpCodes::Jfsc | OpCodes::Extended);
        jmp_addr = code.size();
        D_putInstruction((OpCodes::Type)0);
        threadAST(node->children.second, node);
        code[jmp_addr] = (OpCodes::Type)code.size();
        break;
        
      case ASTNodeType::Or:
        D_putInstruction(OpCodes::Jtsc | OpCodes::Extended);
        jmp_addr = code.size();
        D_putInstruction((OpCodes::Type)0);
        threadAST(node->children.second, node);
        code[jmp_addr] = (OpCodes::Type)code.size();
        break;
      
      case ASTNodeType::If:
        
        D_putInstruction(OpCodes::Jf | OpCodes::Extended);
        jmp_addr = code.size();
        D_putInstruction((OpCodes::Type)0);
        
        if(node->children.second->type == ASTNodeType::Else){
          threadAST(node->children.second->children.first, node);
          if(!node->isValue() && node->children.second->children.first->isValue()){
            D_putInstruction(OpCodes::Pop);
          }
          D_putInstruction(OpCodes::Jmp | OpCodes::Extended);
          code[jmp_addr] = (OpCodes::Type)code.size() + 1;
          jmp_addr = code.size();
          D_putInstruction((OpCodes::Type)0);
          threadAST(node->children.second->children.second, node);
          if(!node->isValue() && node->children.second->children.second->isValue()){
            D_putInstruction(OpCodes::Pop);
          }
          code[jmp_addr] = (OpCodes::Type)code.size();
        }else{
          threadAST(node->children.second, node);
          if(node->children.second->isValue()){
            D_putInstruction(OpCodes::Pop);
          }
          D_putInstruction(OpCodes::Jmp | OpCodes::Extended);
          code[jmp_addr] = (OpCodes::Type)code.size() + 1;
          jmp_addr = code.size();
          D_putInstruction((OpCodes::Type)0);
          D_putInstruction(OpCodes::Push);
          code[jmp_addr] = (OpCodes::Type)code.size();
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
        D_putInstruction(OpCodes::Write | OpCodes::Borrowed);
        break;
      case ASTNodeType::AddAssign:
        D_putInstruction(OpCodes::Add | OpCodes::Borrowed);
        break;
      case ASTNodeType::SubAssign:
        D_putInstruction(OpCodes::Sub | OpCodes::Borrowed);
        break;
      case ASTNodeType::MulAssign:
        D_putInstruction(OpCodes::Mul | OpCodes::Borrowed);
        break;
      case ASTNodeType::DivAssign:
        D_putInstruction(OpCodes::Div | OpCodes::Borrowed);
        break;
      case ASTNodeType::ModAssign:
        D_putInstruction(OpCodes::Mod | OpCodes::Borrowed);
        break;
      case ASTNodeType::AppendAssign:
        D_putInstruction(OpCodes::Append | OpCodes::Borrowed);
        break;
      default:
        assert(false);
      }
    }
    break;
    
  default:
    switch(node->type){
    case ASTNodeType::CodeBlock:
      {
        auto guard = this->newScope();
        threadAST(node->children.first, node);
      }
      break;
      
    case ASTNodeType::Array:
      if(node->child->type == ASTNodeType::Nop){
        D_putInstruction(OpCodes::CreateArray);
      }else{
        OpCodes::Type elems = 0;
        for(auto it = node->child->exprListIterator(); it != nullptr; ++it){
          if(it->type == ASTNodeType::Nop) continue;
          threadAST(it.get(), node);
          ++elems;
        }
        D_putInstruction(OpCodes::CreateArray | OpCodes::Extended | OpCodes::Int);
        D_putInstruction(elems);
      }
      break;
    
    case ASTNodeType::Table:
      if(node->child->type == ASTNodeType::Nop){
        D_putInstruction(OpCodes::CreateTable);
      }else{
        OpCodes::Type elems = 0;
        for(auto it = node->child->exprListIterator(); it != nullptr; ++it){
          if(it->type == ASTNodeType::KeyValuePair){
            threadAST(it->children.first, node);
            threadAST(it->children.second, node);
            ++elems;
          }else if(it->type == ASTNodeType::Nop){
            continue;
          }else assert(false);
        }
        D_putInstruction(OpCodes::CreateTable | OpCodes::Extended | OpCodes::Int);
        D_putInstruction(elems);
      }
      break;
      
    case ASTNodeType::While:
      {
        unsigned begin_addr = code.size();
        
        threadAST(node->children.first, node);
        
        D_putInstruction(OpCodes::Jf | OpCodes::Extended);
        unsigned end_jmp_addr = code.size();
        D_putInstruction((OpCodes::Type)0);
        
        threadAST(node->children.second, node);
        if(node->children.second->isValue()){
          D_putInstruction(OpCodes::Pop);
        }
        D_putInstruction(OpCodes::Jmp | OpCodes::Extended);
        D_putInstruction((OpCodes::Type)begin_addr);
        code[end_jmp_addr] = (OpCodes::Type)code.size();
      }
      break;
    
    case ASTNodeType::For:
      {
        auto guard = this->newScope();
        {
          ASTNode* stepper = node->children.first;
          OpCodes::Type first_pos = (locals++) | stack_pos_local;
          OpCodes::Type second_pos = (locals++) | stack_pos_local;
          if(stepper->type == ASTNodeType::ExprList){
            var_allocs->direct()[stepper->children.first->string_value] = first_pos;
            stepper = stepper->children.second;
          }
          var_allocs->direct()[stepper->children.first->string_value] = second_pos;
          
          threadAST(stepper->children.second, node);
          D_putInstruction(OpCodes::BeginIter | OpCodes::Extended | OpCodes::Extended2);
          D_putInstruction(first_pos);
          D_putInstruction(second_pos);
        }
        
        unsigned begin_addr = code.size();
        D_putInstruction(OpCodes::NextOrJmp | OpCodes::Extended);
        unsigned end_jmp_addr = code.size();
        D_putInstruction((OpCodes::Type)0);
        
        threadAST(node->children.second, node);
        if(node->children.second->isValue()){
          D_putInstruction(OpCodes::Pop);
        }
        D_putInstruction(OpCodes::Jmp | OpCodes::Extended);
        D_putInstruction((OpCodes::Type)begin_addr);
        code[end_jmp_addr] = (OpCodes::Type)code.size();
      }
      break;
      
    case ASTNodeType::Function:
      {
        auto var_alloc = new VarAllocMap(nullptr);
        OpCodes::Type args = 0;
        for(auto it = node->children.first->exprListIterator(); it != nullptr; ++it){
          (*var_alloc)[it->string_value] = args++;
        }
        
        auto func = generate_(
          std::unique_ptr<ASTNode>(node->children.second),
          errors, var_alloc, var_allocs
        );
        if(func == nullptr){
          D_putInstruction(OpCodes::Nop);
          break;
        }
        
        node->children.second = nullptr;
        D_putInstruction(OpCodes::Push | OpCodes::Extended);
        D_putInstruction((OpCodes::Type)constants.emplace(func) | stack_pos_const);
        
        if(func->captures > 0){
          auto& base = static_cast<VectorMapBase&>(var_alloc->base());
          for(int i = 0; i < func->captures; ++i){
            D_putInstruction(OpCodes::Push | OpCodes::Extended);
            D_putInstruction((*var_allocs)[base[i + args].first]);
          }
          D_putInstruction(OpCodes::CreateClosure | OpCodes::Extended | OpCodes::Int);
          D_putInstruction((OpCodes::Type)func->captures);
        }
        
        delete var_alloc;
      }
      break;
      
    case ASTNodeType::Seq:
      threadAST(node->children.first, node);
      if(node->children.first->isValue()){
        D_putInstruction(OpCodes::Pop);
      }
      threadAST(node->children.second, node);
      break;
    
    case ASTNodeType::Var:
      {
        auto subnode = node->child;
        
        threadAST(subnode->children.second, subnode);
        auto it = var_allocs->direct()
          .find(subnode->children.first->string_value);
        OpCodes::Type stack_pos;
        if(it == var_allocs->direct().end()){
          stack_pos = (locals++) | stack_pos_local;
          var_allocs->direct()[subnode->children.first->string_value] = stack_pos;
        }else{
          stack_pos = it->second;
        }
        D_putInstruction(OpCodes::Write | OpCodes::Extended);
        D_putInstruction(stack_pos);
      }
      break;
    
    case ASTNodeType::Return:
      threadAST(node->child, node);
      D_putInstruction(OpCodes::Return);
      break;
    
    case ASTNodeType::Recurse:
      D_putInstruction(OpCodes::Push);
      if(node->child->type == ASTNodeType::Nop){
        D_putInstruction(OpCodes::Recurse);
      }else{
        OpCodes::Type elems = 0;
        for(auto it = node->child->exprListIterator(); it != nullptr; ++it){
          if(it->type == ASTNodeType::Nop) continue;
          threadAST(it.get(), node);
          ++elems;
        }
        D_putInstruction(OpCodes::Recurse | OpCodes::Extended);
        D_putInstruction(elems);
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
          D_putInstruction(OpCodes::Print);
        }else if(num > 1){
          D_putInstruction(OpCodes::Print | OpCodes::Extended | OpCodes::Int);
          D_putInstruction((OpCodes::Type)num);
        }else /*nop*/;
      }
      break;
      
    case ASTNodeType::Assert:
      if(node->child->type == ASTNodeType::ExprList){
        threadAST(node->child->children.second, node);
        threadAST(node->child->children.first, node);
        D_putInstruction(OpCodes::Assert | OpCodes::Alt1);
      }else{
        threadAST(node->child, node);
        D_putInstruction(OpCodes::Assert);
      }
      break;
      
    case ASTNodeType::Index:
      {
        threadAST(node->children.first, node);
        
        if(node->children.second->type == ASTNodeType::ExprList){
          threadAST(node->children.second->children.first, node);
          threadAST(node->children.second->children.second, node);
          D_putInstruction(OpCodes::Slice);
        }else{
          threadAST(node->children.second, node);
          D_putInstruction(OpCodes::Get);
        }
      }
      break;
      
    case ASTNodeType::Call:
      threadAST(node->children.first, node);
      if(node->children.second->type == ASTNodeType::Nop){
        D_putInstruction(OpCodes::Call);
      }else{
        OpCodes::Type elems = 0;
        for(auto it = node->children.second->exprListIterator(); it != nullptr; ++it){
          if(it->type == ASTNodeType::Nop) continue;
          threadAST(it.get(), node);
          ++elems;
        }
        D_putInstruction(OpCodes::Call | OpCodes::Extended);
        D_putInstruction(elems);
      }
      break;
      
    case ASTNodeType::Nop:
      D_putInstruction(OpCodes::Push);
      break;
      
    default:
      assert(false);
    }
  }
}

void CodeGenerationContext::threadRexpr(ASTNode* node, ASTNode* prev_node){
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
      D_putInstruction(OpCodes::Borrow | OpCodes::Extended);
      D_putInstruction(it->second);
    }
    break;
  case ASTNodeType::Index:
    this->threadRexpr(node->children.first, node);
    this->threadAST(node->children.second, node);
    D_putInstruction(OpCodes::Borrow | OpCodes::Borrowed);
    break;
  }
}

void CodeGenerationContext::threadInsRexpr(ASTNode* node, ASTNode* prev_node){
  if(node->type == ASTNodeType::Index){
    this->threadRexpr(node->children.first, node);
    this->threadAST(node->children.second, node);
    D_putInstruction(OpCodes::Borrow | OpCodes::Borrowed | OpCodes::Alt1);
  }else{
    if(node->type == ASTNodeType::Identifier){
      errors->emplace_back(dynSprintf(
        "line %d: Insertion to variable", node->pos.first
      ));
    }
    D_putInstruction(OpCodes::Nop);
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
