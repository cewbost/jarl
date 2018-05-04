#include "function.h"

#include "code_generator.h"

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
    decltype(context_var_allocs) cva = nullptr
  )
  : var_allocs(std::move(va)),
    context_var_allocs(cva),
    errors(err),
    stack_size(0),
    arguments(0) {}
  
  void putInstruction(OpCodeType op, int pos);
  void removeStackTop(int pos);
  
  void threadAST(ASTNode*, ASTNode* = nullptr);
};

namespace {
  
  Function* generate_(
    ASTNode* parse_tree,
    std::vector<std::unique_ptr<char[]>>* errors,
    VarAllocMap* var_allocs,
    VarAllocMap* context_var_allocs = nullptr
  ){
    assert(var_allocs != nullptr);
    
    int args = var_allocs->size();
    
    ThreadingContext context(
      errors,
      std::move(var_allocs),
      context_var_allocs
    );
    
    auto pos = parse_tree->pos.first;
    context.code_positions.emplace_back(pos, 0);
    context.threadAST(parse_tree);
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
          op = (op & ~stack_pos_local) + context.constants.size() + args;
        }else if(op & stack_pos_const){
          op = (op & ~stack_pos_const) + args;
        }
      }
    }
    
    delete parse_tree;
    
    return new Function(
      std::move(context.code),
      std::move(context.constants),
      std::move(context.code_positions),
      context.arguments
    );
  }
}

void ThreadingContext::putInstruction(OpCodeType op, int pos){
  if(this->code_positions.back().first != pos){
    this->code_positions.emplace_back(pos, this->code.size());
  }
  this->code.push_back(op);
}

void ThreadingContext::removeStackTop(int pos){
  if(code.size() >= 2
  && (code[code.size() - 2] & ~(Op::Head & ~Op::Extended))
  == (Op::Push | Op::Extended)){
    code.pop_back();
    code.pop_back();
  }else if(code.back() == Op::Push
  || code.back() == Op::PushFalse
  || code.back() == Op::PushTrue){
    code.pop_back();
  }else{
    putInstruction(Op::Pop, pos);
  }
}

void ThreadingContext::threadAST(ASTNode* node, ASTNode* prev_node){
  #define D_putInstruction(ins) this->putInstruction(ins, node->pos.first)
  #define D_removeStackTop() this->removeStackTop(node->pos.first)
  
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
            break;
          }
          
          it = context_var_allocs->find(node->string_value);
          
          if(it == context_var_allocs->end()){
            errors->emplace_back(dynSprintf(
              "line %d: Undeclared identifier '%s'.",
              node->pos.first,
              node->string_value->str()
            ));
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
        threadAST(node->children.second, node);
        code[jmp_addr] = (OpCodeType)code.size();
        break;
        
      case ASTNodeType::Or:
        D_putInstruction(Op::Jtsc | Op::Extended);
        jmp_addr = code.size();
        D_putInstruction((OpCodeType)0);
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
          code[jmp_addr] = (OpCodeType)code.size();
        }
        break;
        
      default:
        assert(false);
      }
    }
    break;
    
  case ASTNodeType::DefineExpr:
    switch(node->type){
    case ASTNodeType::Define:
      {
        if(node->children.first->type != ASTNodeType::Identifier){
          errors->emplace_back(
            dynSprintf("line %d: Definition of rvalue", node->pos.first)
          );
        }
        
        auto old_stack_size = stack_size;
        threadAST(node->children.second, node);
        auto it = var_allocs->direct().find(node->children.first->string_value);
        if(it != var_allocs->direct().end()){
          D_putInstruction(Op::Write | Op::Extended | Op::Dest);
          D_putInstruction(it->second);
          D_putInstruction(Op::Push | Op::Extended);
          D_putInstruction(it->second);
        }else{
          var_allocs->direct()[node->children.first->string_value] =
            old_stack_size | stack_pos_local;
          D_putInstruction(Op::Push);
        }
      }
      break;
    default:
      assert(false);
    }
    break;
    
  case ASTNodeType::AssignExpr:
    {
      if(node->children.first->type != ASTNodeType::Identifier){
        errors->emplace_back(
          dynSprintf("line %d: Assignment to rvalue.", node->pos.first)
        );
      }
      
      auto stack_pos = (OpCodeType)(*var_allocs)[node->children.first->string_value];
      threadAST(node->children.second, node);
      switch(node->type){
      case ASTNodeType::Assign:
        D_putInstruction(Op::Write | Op::Extended | Op::Dest);
        break;
      case ASTNodeType::AddAssign:
        D_putInstruction(Op::Add | Op::Extended | Op::Dest);
        break;
      case ASTNodeType::SubAssign:
        D_putInstruction(Op::Sub | Op::Extended | Op::Dest);
        break;
      case ASTNodeType::MulAssign:
        D_putInstruction(Op::Mul | Op::Extended | Op::Dest);
        break;
      case ASTNodeType::DivAssign:
        D_putInstruction(Op::Div | Op::Extended | Op::Dest);
        break;
      case ASTNodeType::ModAssign:
        D_putInstruction(Op::Mod | Op::Extended | Op::Dest);
        break;
      case ASTNodeType::AppendAssign:
        D_putInstruction(Op::Append | Op::Extended | Op::Dest);
        break;
        
      default:
        assert(false);
      }
      
      D_putInstruction(stack_pos);
      D_putInstruction(Op::Push | Op::Extended);
      D_putInstruction(stack_pos);
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
        D_removeStackTop();
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
            break;
          }
        }
        
        auto func = generate_(node->children.second, errors, var_alloc, var_allocs);
        
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
      if(node->children.first->type != ASTNodeType::If){
        D_removeStackTop();
      }else{
        D_putInstruction(Op::Pop);
      }
      --stack_size;
      threadAST(node->children.second, node);
      break;
      
    case ASTNodeType::Print:
      threadAST(node->child, node);
      D_putInstruction(Op::Print);
      break;
      
    case ASTNodeType::Index:
      {
        threadAST(node->children.first, node);
        
        if(node->children.second->type == ASTNodeType::Range){
          threadAST(node->children.second->children.first, node);
          threadAST(node->children.second->children.second, node);
          D_putInstruction(Op::Slice);
        }else{
          threadAST(node->children.second, node);
          D_putInstruction(Op::Get);
        }
        
        --stack_size;
      }
      break;
      
    case ASTNodeType::Nop:
      D_putInstruction(Op::Push);
      break;
      
    case ASTNodeType::Range:
      errors->emplace_back(dynSprintf(
        "line %d: Range generators not yet supported.", node->pos.first
      ));
      break;
      
    default:
      assert(false);
    }
  }
  
  #undef D_putInstruction
  #undef D_removeStackTop
}

namespace CodeGenerator {
  
  Function* generate(
    ASTNode* parse_tree,
    std::vector<std::unique_ptr<char[]>>* errors
  ){
    VarAllocMap* var_allocs = new VarAllocMap(nullptr);
    auto ret = generate_(parse_tree, errors, var_allocs);
    delete var_allocs;
    return ret;
  }
}
