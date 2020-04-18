#include <set>                           // stl 集合容器
#include <string>
#include "llvm/Pass.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

cl::list<std::string> FunctionList(
        "instrument",                           // 函数内容 指令
        cl::desc("Functions to instrument"),    //
        cl::value_desc("function name"),
        cl::OneOrMore);

namespace {
    class InstrumentFunctions : public ModulePass { // 继承 ModulePass
        public:

            // name of instrumentation functions
            const char *LOG_FUNCTION_STR = "log_function_call";  // 日志打印函数调用
            const char *INIT_FUNCTION_STR = "init";              // 初始化函数
            /* const char *LOG_VARIABLE_STR = "log_variable_change"; */

            static char ID;

            std::set<std::string> funcsToInst; // 以函数名 记录函数是否已修改

            InstrumentFunctions() : ModulePass(ID) {
                for (unsigned i = 0; i != FunctionList.size(); ++i) {
                    funcsToInst.insert(FunctionList[i]);
                }
            }

            bool runOnModule(Module &M) override {
                // 声明日志函数
                declare_log_functions(M); // 后面定义
                    
                for (Module::iterator mi = M.begin(); mi != M.end(); ++mi) {
                    // 遍历模块内部的每一个函数
                    Function &f = *mi;
                    std::string fname = f.getName();            // 函数名
                    /* errs().write_escaped(fname) << "\n"; */
                    if (fname == "main") {
                        initializeLogger(M, f);                 // 在main 主函数 加入自己的初始化日志函数
                    }
                    if (funcsToInst.count(fname) != 0) {
                        instrumentFunction(M, f);               // 修改其他函数
                    }
                }
                return true;
            }
            
            // 在主函数 加入自己的代码
            void initializeLogger(Module &M, Function &f) {
                // main 函数入口 代码块
                BasicBlock &entryBlock = f.getEntryBlock();
                
                // 自己定义的 函数
                Function *initFunction = M.getFunction(INIT_FUNCTION_STR); // 初始化函数
                
                // 在主函数 第一个block代码块后 加入自己写的函数
                CallInst::Create(initFunction, "", entryBlock.getFirstNonPHI());
            }
            
            // 修改其他函数
            void instrumentFunction(Module &M, Function &f) {
                // 函数内部第一个代码块的 第一个指令
                BasicBlock &entryBlock = f.getEntryBlock();
                Instruction *firstInstr = entryBlock.getFirstNonPHI();

                IRBuilder<> builder(firstInstr);
                // 该函数的指针变量
                Value *strPointer = builder.CreateGlobalStringPtr(f.getName());
                
                // 自己定义的 日志函数
                Function *logFunction = M.getFunction(LOG_FUNCTION_STR);

                std::vector<Value *> args;
                args.push_back(strPointer); // 生成日志函数 的参数列表
                
                // 创建自己的日志函数 并传入  本函数的函数指针
                CallInst::Create(logFunction, args, "", entryBlock.getFirstNonPHI());
            }



            void declare_log_functions(Module &m) {
                // 代码上下文
                LLVMContext &C = m.getContext();
                    
                // void type
                Type *voidTy = Type::getVoidTy(C);

                // 64 bit integer
                Type *IntTy64 = Type::getInt64Ty(C);
                
                // 字符串类型
                Type *StringType = Type::getInt8PtrTy(C);

                bool isVarArg = false;

                /* std::vector<Type*> variable_change_params; */
                /* variable_change_params.push_back(StringType); */
                /* variable_change_params.push_back(IntTy64); */
                /* FunctionType *variable_change_type = FunctionType::get(
                 * voidTy, variable_change_params, isVarArg); */
                
                // 函数参数类型
                std::vector<Type*> functionCallParams;
                functionCallParams.push_back(StringType);
                
                // 函数调用类型   (函数指针, 函数参数列表，)
                FunctionType *functionCallType = FunctionType::get(
                        voidTy, functionCallParams, isVarArg
                        );
                
                // 初始化函数类型
                FunctionType *initFunctionType = FunctionType::get(
                        IntTy64, isVarArg
                        );

                // 在模块内部插入 自己定义的函数 insert functions to module
                m.getOrInsertFunction(LOG_FUNCTION_STR, functionCallType);   // 日志函数（函数参数类型）
                m.getOrInsertFunction(INIT_FUNCTION_STR, initFunctionType);  // 初始化函数

                /* m.getOrInsertFunction(LOG_VARIABLE_STR, variable_change_type); */
            }
    }; // end of struct
}  // end of anonymous namespace

char InstrumentFunctions::ID = 0;

// 注册该pass
static RegisterPass<InstrumentFunctions> X("instrument_function_calls",
        "Instrument Function Calls Pass",
        false /* Modifies CFG */,
        false /* Non Analysis Pass */);
