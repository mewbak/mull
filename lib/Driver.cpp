#include "Driver.h"
#include "Compiler.h"
#include "Config.h"
#include "Context.h"
#include "ModuleLoader.h"

#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"

/// FIXME: Should be abstract
#include "TestFinders/SimpleTestFinder.h"

/// FIXME: Should be abstract
#include "TestRunners/SimpleTestRunner.h"

/// FIXME: Should be abstract
#include "MutationOperators/AddMutationOperator.h"

//using namespace llvm;
using namespace Mutang;

/// Populate Mutang::Context with modules using
/// ModulePaths from Mutang::Config.
/// Mutang::Context should be populated using ModuleLoader
/// so that we could inject modules from string for testing purposes

/// Having Mutang::Context in place we could instantiate TestFinder and find all tests
/// Using same TestFinder we could find mutation points, apply them sequentially and
/// run tests/mutants using newly created TestRunner

/// This method should return (somehow) results of the tests/mutants execution
/// So that we could easily plug in some TestReporter

/// UPD: The method returns set of results
/// Number of results equals to a number of tests
/// Each result contains result of execution of an original test and
/// all the results of each mutant within corresponding MutationPoint

void Driver::Run() {
  Context Ctx;
  for (auto ModulePath : Cfg.GetBitcodePaths()) {
    auto Module = Loader.loadModuleAtPath(ModulePath);
    assert(Module && "Can't load module");
    Ctx.addModule(std::move(Module));
  }

  /// FIXME: Should come from the outside
  AddMutationOperator MutOp;
  std::vector<MutationOperator *> MutationOperators;
  MutationOperators.push_back(&MutOp);

  Compiler Compiler;

  SimpleTestFinder TestFinder(Ctx);
  for (auto Test : TestFinder.findTests()) {
    for (auto Testee : TestFinder.findTestees(*Test)) {
      for (auto &MutationPoint : TestFinder.findMutationPoints(MutationOperators, *Testee)) {
        SimpleTestRunner Runner;
        SimpleTestRunner::ObjectFiles ObjectFiles;

        MutationPoint->applyMutation();

        /// Recompile all the modules all the time
        /// I assume it is incredibly slow
        /// If so, then this is a perfect place
        /// to start playing with optimizations
        for (auto &M : Ctx.getModules()) {
          ObjectFiles.push_back(Compiler.CompilerModule(M.get()));
        }

        /// Rollback mutation once we have compiled the module
        MutationPoint->revertMutation();

        Runner.runTest(Test, ObjectFiles);
      }
    }
  }
}
