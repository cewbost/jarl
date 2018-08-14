#include "function.h"

#include "code_generator.h"

#ifndef NDEBUG
#include <cstdio>
//#define PRINT_CODE
#else
#undef PRINT_CODE
#endif

using namespace CodeGenerator;

constexpr OpCodeType stack_pos_local = 0x8000;
constexpr OpCodeType stack_pos_const = 0x4000;

struct ThreadingContext {
  std::vector<OpCodeType> code;
  std::vector<std::pair<int, int>> code_positions;
  
  VarAllocMap *var_allocs, *context_var_allocs;
  VectorSet<TypedValue> constants;
  
  int stack_size;
  int arguments;
  
  std::vector<std::unique_ptr<char[]>> *errors;
  
  ThreadingContext(
    decltype(errors) err,
    decltype(var_allocs) va = nullptr,
    decltype(context_var_allocs) cva = nullptr,
    int args = 0
  )
  : var_allocs(std::move(va)),
    context_var_allocs(cva),
    errors(err),
    stack_size(0),
    arguments(args) {}
  
  void putInstruction(OpCodeType op, int pos);
  
  void threadAST(ASTNode*, ASTNode* = nullptr);
  void threadRexpr(ASTNode* node, ASTNode* prev_node);
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
    
    auto pos = parse_tree->pos.first;
    context.code_positions.emplace_back(pos, 0);
    context.threadAST(parse_tree.get());
    if(!errors->empty()) return nullptr;
    context.putInstruction(Op::Return, pos);
    
    //correct stack positions
    int extended = 0;
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
        if(op & stack_pos_local){
          op = (op & ~stack_pos_local) + context.constants.size() + context.arguments;
        }else if(op & stack_pos_const){
          op = (op & ~stack_pos_const) + context.arguments;
        }
      }
    }
    
    auto func = new Function(
      std::move(context.code),
      std::move(context.constants),
      std::move(context.code_positions),
      context.arguments
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
            errors->emplace_back(dynSprintf(
              "line %d: Undeclared identifier '%s'.",
              node->pos.first,
              node->string_value->str()
            ));
            D_putInstruction(Op::Nop);
            break;
          }
          
          it = context_var_allocs->find(node->string_value);
          
          if(it == context_var_allocs->end()){
            errors->emplace_back(dynSprintf(
              "line %d: Undeclared identifier '%s'.",
              node->pos.first,
              node->string_value->str()
            ));
            D_putInstruction(Op::Nop);
            break;
          }
          
          var_allocs->base()[node->string_value] = arguments;
          D_putInstruction(arguments);
          ++arguments;
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
      threadRexpr(node->children.first, node);
      threadAST(node->children.second, node);
      
      switch(node->type){
      case ASTNodeType::Assign:
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
        var_allocs = new VarAllocMap(var_allocs);
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
        
        auto old_allocs = var_allocs;
        var_allocs = var_allocs->get_delegate();
        delete old_allocs;
      }
      break;
      
    case ASTNodeType::Array:
      
      if(node->child->type != ASTNodeType::Nop){
        if(node->child->type == ASTNodeType::ExprList){
          ASTNode* t = node->child;
          OpCodeType elems = 0;
          
          for(;;){
            threadAST(t->children.first, node);
            ++elems;
            if(t->children.second->type == ASTNodeType::ExprList){
              t = t->children.second;
            }else{
              threadAST(t->children.second, node);
              ++elems;
              break;
            }
          }
          
          D_putInstruction(Op::CreateArray | Op::Extended | Op::Int);
          D_putInstruction(elems);
          stack_size -= elems - 1;
        }else if(node->child->type == ASTNodeType::Range){
          threadAST(node->child->children.first, node);
          threadAST(node->child->children.second, node);
          D_putInstruction(Op::CreateRange);
          --stack_size;
        }else{
          threadAST(node->child, node);
          D_putInstruction(Op::CreateArray | Op::Extended | Op::Int);
          D_putInstruction(1);
        }
      }else{
        D_putInstruction(Op::CreateArray);
        ++stack_size;
      }
      
      break;
      
    case ASTNodeType::For:
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
      
    case ASTNodeType::Function:
      {
        //some checking might be nessecary for the function being well formed
        
        auto var_alloc = new VarAllocMap(nullptr);
        OpCodeType std_args = 0;
        for(
          auto it = node->children.first->exprListIterator();
          it != node->children.second->exprListIteratorEnd();
          ++it
        ){
          if(it->type == ASTNodeType::Identifier){
            (*var_alloc)[it->string_value] = std_args++;
          }else{
            errors->emplace_back(dynSprintf(
              "line %d: Invalid function parameter",
              node->pos.first
            ));
            D_putInstruction(Op::Nop);
            break;
          }
        }
        
        auto func = generate_(
          std::unique_ptr<ASTNode>(node->children.second),
          errors, var_alloc, var_allocs
        );
        
        node->children.second = nullptr;
        D_putInstruction(Op::Push | Op::Extended);
        D_putInstruction((OpCodeType)constants.emplace(func) | stack_pos_const);
        ++stack_size;
        
        if(func->arguments > std_args){
          auto& base = static_cast<VectorMapBase&>(var_alloc->base());
          for(int i = std_args; i < func->arguments; ++i){
            D_putInstruction(Op::Push | Op::Extended);
            D_putInstruction((*var_allocs)[base[i].first]);
            D_putInstruction(Op::Apply | Op::Extended | Op::Int);
            D_putInstruction((OpCodeType)std_args);
          }
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
        switch(node->child->type){
        case ASTNodeType::Assign:
          {
            auto subnode = node->child;
            if(subnode->children.first->type != ASTNodeType::Identifier){
              errors->emplace_back(
                dynSprintf("line %d: Expected identifier", subnode->pos.first)
              );
              D_putInstruction(Op::Nop);
              break;
            }
            
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
        default:
          errors->emplace_back(dynSprintf(
            "line %d: Invalid variable declaration syntax",
            node->pos.first
          ));
          D_putInstruction(Op::Nop);
          break;
        }
      }
      break;
    
    case ASTNodeType::Print:
      {
        int num = 0;
        for(
          auto it = node->child->exprListIterator();
          it != node->child->exprListIteratorEnd();
          ++it
        ){
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
      {
        if(node->child->type == ASTNodeType::ExprList){
          if(node->child->children.second->type == ASTNodeType::ExprList){
            goto num_params_error;
          }
          threadAST(node->child->children.second, node);
          threadAST(node->child->children.first, node);
          D_putInstruction(Op::Assert | Op::Alt);
          --stack_size;
        }else{
          if(node->child->type == ASTNodeType::Nop){
            goto num_params_error;
          }
          threadAST(node->child, node);
          D_putInstruction(Op::Assert);
        }
        break;
      
      num_params_error:
        errors->emplace_back(dynSprintf(
          "line %d: Wrong number of parameters to assertion",
          node->pos.first
        ));
        D_putInstruction(Op::Nop);
        break;
      }
      
    case ASTNodeType::Index:
      {
        threadAST(node->children.first, node);
        
        if(node->children.second->type == ASTNodeType::ExprList){
          if(node->children.second->children.first->type == ASTNodeType::ExprList
          || node->children.second->children.second->type == ASTNodeType::ExprList){
            errors->emplace_back(dynSprintf(
              "line %d: too many parameters in index",
              node->pos.first
            ));
            D_putInstruction(Op::Nop);
            break;
          }
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
      
    case ASTNodeType::Nop:
      D_putInstruction(Op::Push);
      ++stack_size;
      break;
      
    case ASTNodeType::Range:
      errors->emplace_back(dynSprintf(
        "line %d: Range generators not yet supported.", node->pos.first
      ));
      D_putInstruction(Op::Nop);
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
      errors->emplace_back(dynSprintf(
        "line %d: Undeclared identifier '%s'.",
        node->pos.first, node->string_value
      ));
      D_putInstruction(Op::Nop);
      break;
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
  default:
    errors->emplace_back(dynSprintf(
      "line %d: Assignment to rvalue", node->pos.first
    ));
    D_putInstruction(Op::Nop);
    break;
  }
}

#undef D_putInstruction

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
