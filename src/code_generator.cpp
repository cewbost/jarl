#include "function.h"

#include "code_generator.h"

using namespace CodeGenerator;

constexpr OpCodeType stack_pos_local = 0x8000;
constexpr OpCodeType stack_pos_const = 0x4000;

struct ThreadingContext {
  std::vector<OpCodeType> code;
  std::vector<std::pair<int, int>> code_positions;
  
  std::unique_ptr<VarAllocMap> var_allocs, context_var_allocs;
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
    context_var_allocs(std::move(cva)),
    errors(err),
    stack_size(0),
    arguments(0) {}
  
  void putInstruction(OpCodeType op, int pos);
  
  void threadAST(ASTNode*, ASTNode* = nullptr, bool = true);
};

void ThreadingContext::putInstruction(OpCodeType op, int pos){
  if(this->code_positions.back().first != pos){
    this->code_positions.emplace_back(pos, this->code.size());
  }
  this->code.push_back(op);
}

void ThreadingContext::threadAST(ASTNode* node, ASTNode* prev_node, bool ret){
  #define D_putInstruction(ins) this->putInstruction(ins, node->pos.first)
  
  switch(static_cast<ASTNodeType>(static_cast<unsigned>(node->type) & ~0xff)){
  case ASTNodeType::Error:
    assert(false);
    break;
    
  case ASTNodeType::Value:
    if(!ret) break;
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
    if(!ret){
      D_putInstruction(Op::Pop);
      --stack_size;
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
    if(!ret){
      D_putInstruction(Op::Pop);
      stack_size -= 2;
    }else --stack_size;
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
        code[jmp_addr] = (OpCodeType)code.size() - 1;
        
        if(!ret){
          D_putInstruction(Op::Pop);
          --stack_size;
        }
        break;
      case ASTNodeType::Or:
        D_putInstruction(Op::Jtsc | Op::Extended);
        jmp_addr = code.size();
        D_putInstruction((OpCodeType)0);
        threadAST(node->children.second, node);
        code[jmp_addr] = (OpCodeType)code.size() - 1;
        
        if(!ret){
          D_putInstruction(Op::Pop);
          --stack_size;
        }
        break;
        
      case ASTNodeType::Conditional:
        assert(node->children.second->type == ASTNodeType::Branch);
        
        D_putInstruction(Op::Jf | Op::Extended);
        jmp_addr = code.size();
        D_putInstruction((OpCodeType)0);
        --stack_size;
        
        threadAST(node->children.second->children.first, node);
        --stack_size;
        
        code[jmp_addr] = (OpCodeType)code.size() + 1;
        
        if(ret){
          D_putInstruction(Op::Jmp | Op::Extended);
          jmp_addr = code.size();
          D_putInstruction((OpCodeType)0);
          if(node->children.second->children.second->type == ASTNodeType::Nop){
            D_putInstruction(Op::Push);
            ++stack_size;
          }else{
            threadAST(node->children.second->children.second, node);
          }
          code[jmp_addr] = (OpCodeType)code.size() + 1;
        }else if(node->children.second->children.second->type != ASTNodeType::Nop){
          D_putInstruction(Op::Jmp | Op::Extended);
          jmp_addr = code.size();
          D_putInstruction((OpCodeType)0);
          
          threadAST(node->children.second->children.second, node);
          
          code[jmp_addr] = (OpCodeType)code.size() + 1;
        }
        break;
        
      default:
        assert(false);
      }
    }
    break;
    
  case ASTNodeType::DefineExpr:
    {
      if(node->children.first->type != ASTNodeType::Identifier){
        errors->emplace_back(
          dynSprintf("line %d: Definition of rvalue", node->pos.first)
        );
      }
      
      auto old_stack_size = stack_size;
      threadAST(node->string_branch.next, node);
      auto it = var_allocs->direct().find(node->string_branch.value);
      if(it != var_allocs->direct().end()){
        D_putInstruction(Op::Write | Op::Extended | Op::Dest);
        D_putInstruction(it->second);
        if(ret){
          D_putInstruction(Op::Push | Op::Extended | Op::Dest);
          D_putInstruction(it->second);
        }else --stack_size;
      }else{
        var_allocs->direct()[node->string_branch.value] =
          old_stack_size | stack_pos_local;
      }
    }
    
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
      if(ret){
        D_putInstruction(Op::Push | Op::Extended | Op::Dest);
        D_putInstruction(stack_pos);
      }else{
        --stack_size;
      }
    }
    break;
    
  default:
    switch(node->type){
    case ASTNodeType::CodeBlock:
      
      var_allocs.reset(new VarAllocMap(var_allocs.release()));
      threadAST(node->children.first, node);
      
      if(auto size = var_allocs->direct().size(); ret){
        if(size > 0){
          if(size == 1){
            D_putInstruction(Op::Reduce);
          }else{
            D_putInstruction(Op::Reduce | Op::Extended);
            D_putInstruction(static_cast<OpCodeType>(size));
          }
          stack_size -= size;
        }
      }else{
        if(size == 0){
          D_putInstruction(Op::Pop);
        }else{
          D_putInstruction(Op::Pop | Op::Extended);
          D_putInstruction(static_cast<OpCodeType>(size + 1));
        }
      }
      
      var_allocs.reset(var_allocs->get_delegate());
      
      break;
      
    case ASTNodeType::Array:
      
      if(!ret) break;
      
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
      
    case ASTNodeType::While:
      {
        if(ret){
          D_putInstruction(Op::Push);
          ++stack_size;
        }
        unsigned begin_addr = code.size() - 1;
        
        threadAST(node->children.first, node);
        
        D_putInstruction(Op::Jf | Op::Extended);
        --stack_size;
        unsigned end_jmp_addr = code.size();
        D_putInstruction((OpCodeType)0);
        
        threadAST(node->children.second, node);
        if(ret){
          D_putInstruction(Op::Reduce);
          --stack_size;
        }
        D_putInstruction(Op::Jmp | Op::Extended);
        code[end_jmp_addr] = (OpCodeType)code.size();
        D_putInstruction((OpCodeType)begin_addr);
      }
      break;
      
    case ASTNodeType::Function:
      {
        //some checking might be nessecary for the function being well formed
        
        if(!ret) break;
        
        //Do something
        /*if(!ret) break;
        
        VarAllocMap* var_alloc = new VarAllocMap(nullptr);
        
        OpCodeType std_args = 0;
        if(node->children.first->type != ASTNodeType::Nop){
          ASTNode* t = node->children.first;
          do{
            (*var_alloc)[t->string_branch.value] = std_args;
            ++std_args;
            t = t->string_branch.next;
          }while(t->type != ASTNodeType::Nop);
        }
        
        delete node->children.first;
        node->children.first = nullptr;
        
        auto* func = new Function(node->children.second, errors, var_alloc, var_allocs);
        
        node->children.second = nullptr;
        this->putInstruction_(Op::Push | Op::Extended, node->pos.first);
        this->putInstruction_(
          (OpCodeType)constants.emplace(func) | stack_pos_const_,
          node->pos.first
        );
        ++(*stack_size);
        
        if(func->arguments > std_args){
          VectorMapBase& base = var_alloc->base();
          for(int i = std_args; i < func->arguments; ++i){
            this->putInstruction_(Op::Push | Op::Extended, node->pos.first);
            this->putInstruction_((*var_allocs)[base[i].first], node->pos.first);
            this->putInstruction_(Op::Apply | Op::Extended | Op::Int, node->pos.first);
            this->putInstruction_((OpCodeType)std_args, node->pos.first);
          }
        }
        delete var_alloc;*/
      }
      break;
      
    case ASTNodeType::Seq:
      threadAST(node->children.first, node, false);
      threadAST(node->children.second);
      break;
      
    case ASTNodeType::VarDecl:
      assert(false);
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
        
        if(!ret){
          D_putInstruction(Op::Pop);
          stack_size -= 2;
        }else --stack_size;
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
}

namespace CodeGenerator {
  
  Function* generate(
    std::unique_ptr<ASTNode> parse_tree,
    std::vector<std::unique_ptr<char[]>>* errors,
    std::unique_ptr<VarAllocMap> var_allocs,
    std::unique_ptr<VarAllocMap> context_var_allocs
  ){
    int args;
    bool va = false;
    
    if(var_allocs){
      args = var_allocs->size();
      va = true;
    }else{
      args = 0;
      var_allocs.reset(new VarAllocMap(nullptr));
    }
    
    ThreadingContext context(
      errors,
      std::move(var_allocs),
      std::move(context_var_allocs)
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
          op = (op & ~stack_pos_local) + context.constants.size() + args;
        }else if(op & stack_pos_const){
          op = (op & ~stack_pos_const) + args;
        }
      }
    }
    
    return new Function(
      std::move(context.code),
      std::move(context.constants),
      std::move(context.code_positions)
    );
  }
}
