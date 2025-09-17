#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"
#include "stdbool.h"

#define BUILD_FOLDER "build/"
#define SRC_FOLDER   "src/"

#define nob_cc_include_path(cmd, path) cmd_append(cmd, temp_sprintf("-I%s", path))
#define nob_cc_no_link(cmd) cmd_append(cmd, "-c")


bool build_object(Cmd *cmd, const char *src_path, const char *o_path) {
  nob_cc(cmd);
  nob_cc_flags(cmd);
  nob_cmd_append(cmd, "-ggdb");
  nob_cc_include_path(cmd, SRC_FOLDER);
  nob_cc_inputs(cmd, src_path);
  nob_cc_no_link(cmd);
  nob_cc_output(cmd, o_path);

  return cmd_run_sync_and_reset(cmd);
}

int main(int argc, char **argv) {
  NOB_GO_REBUILD_URSELF(argc, argv);

  if (!nob_mkdir_if_not_exists(BUILD_FOLDER)) return 1;

  Nob_Cmd cmd = {0};

  if (!build_object(&cmd, SRC_FOLDER"main.c", BUILD_FOLDER"main.o")) return 1;

  nob_cc(&cmd);
  nob_cc_flags(&cmd);
  nob_cmd_append(&cmd, "-ggdb");
  nob_cc_include_path(&cmd, SRC_FOLDER);
  nob_cc_inputs(&cmd, BUILD_FOLDER"main.o");
  nob_cmd_append(&cmd, "-lm", "-lasound", "-lraylib");
  nob_cc_output(&cmd, BUILD_FOLDER"crynth");
  
  if (!nob_cmd_run(&cmd)) return 1;
  return 0;
}
