#include "Maverick.hh"
#include "OcpSolverImpl.hh"
#include "MaverickUtils/GenericFunction/GenericFunction1AManager.hh"
#include "MaverickUtils/GenericFunction/GenericFunction2AManager.hh"

namespace Maverick {

  std::unique_ptr<Maverick::MaverickOcpSolver> getMaverickOcpSolver(Maverick::MaverickOcp &ocp) {
    return std::unique_ptr<Maverick::MaverickOcpSolver>(new Maverick::OcpSolverImpl(ocp));
  }

  std::unique_ptr<MaverickUtils::GenericFunction1AInterface> getGenericFunction1A() {
    return std::unique_ptr<MaverickUtils::GenericFunction1AManager>(new MaverickUtils::GenericFunction1AManager());
  }

  std::unique_ptr<MaverickUtils::GenericFunction1AInterface> getGenericFunction1A(std::string const &name) {
    return std::unique_ptr<MaverickUtils::GenericFunction1AManager>(new MaverickUtils::GenericFunction1AManager(name));
  }

  std::unique_ptr<MaverickUtils::GenericFunction2AInterface> getGenericFunction2A() {
    return std::unique_ptr<MaverickUtils::GenericFunction2AManager>(new MaverickUtils::GenericFunction2AManager());
  }

  std::unique_ptr<MaverickUtils::GenericFunction2AInterface> getGenericFunction2A(std::string const &name) {
    return std::unique_ptr<MaverickUtils::GenericFunction2AManager>(new MaverickUtils::GenericFunction2AManager(name));
  }

}
