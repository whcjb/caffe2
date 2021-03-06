#ifndef CAFFE2_CORE_INIT_H_
#define CAFFE2_CORE_INIT_H_

#include "caffe2/core/common.h"
#include "gflags/gflags.h"
#include "glog/logging.h"

// This allows us to use legacy gflags namespaces.
#ifndef GFLAGS_GFLAGS_H_
  namespace gflags = google;
#endif

namespace caffe2 {

namespace internal {
class Caffe2InitializeRegistry {
 public:
  typedef bool (*InitFunction)(void);
  static Caffe2InitializeRegistry* Registry() {
    static Caffe2InitializeRegistry gRegistry;
    return &gRegistry;
  }

  void Register(InitFunction function, const char* description) {
    init_functions_.emplace_back(function, description);
  }

  // Run all registered initialization functions. This has to be called AFTER
  // all static initialization are finished and main() has started, since we are
  // using logging.
  bool RunRegisteredInitFunctions() {
    for (const auto& init_pair : init_functions_) {
      LOG(INFO) << "Running init function: " << init_pair.second;
      if (!(*init_pair.first)()) {
        LOG(ERROR) << "Initialization function failed.";
        return false;
      }
    }
    return true;
  }

 private:
  Caffe2InitializeRegistry() : init_functions_() {}
  vector<std::pair<InitFunction, const char*> > init_functions_;
};
}  // namespace internal

class InitRegisterer {
 public:
  InitRegisterer(internal::Caffe2InitializeRegistry::InitFunction function,
                 const char* description) {
    internal::Caffe2InitializeRegistry::Registry()
        ->Register(function, description);
  }
};

#define REGISTER_CAFFE2_INIT_FUNCTION(name, function, description)             \
  namespace {                                                                  \
  ::caffe2::InitRegisterer g_caffe2_initregisterer_name(                       \
      function, description);                                                  \
  }  // namespace

// Initialize the global environment of caffe2.
inline bool GlobalInit(int* pargc, char*** pargv) {
  static bool global_init_was_already_run = false;
  if (global_init_was_already_run) {
    LOG(FATAL) << "GlobalInit has already been called: did you double-call?";
  }
  // Google flags.
  ::gflags::ParseCommandLineFlags(pargc, pargv, true);
  // Google logging.
  ::google::InitGoogleLogging(*(pargv)[0]);
  // Provide a backtrace on segfault.
  ::google::InstallFailureSignalHandler();
  // All other initialization functions.
  bool retval = internal::Caffe2InitializeRegistry::Registry()
      ->RunRegisteredInitFunctions();
  global_init_was_already_run = true;
  return retval;
}


}  // namespace caffe2
#endif  // CAFFE2_CORE_INIT_H_
