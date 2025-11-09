#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"
#include "stdbool.h"

#define BUILD_FOLDER "build/"
#define SRC_FOLDER "src/"
#define THIRD_PARTY_FOLDER "third-party/"

#define nob_cc_include_path(cmd, path) cmd_append(cmd, temp_sprintf("-I%s", path))
#define nob_cc_no_link(cmd) cmd_append(cmd, "-c")
#define nob_cc_my_flags(cmd) cmd_append(cmd, "-Wall", "-Wextra", "-Wno-unused-function", "-ggdb")

typedef struct {
  Nob_String_View *items;
  size_t count;
  size_t capacity;
} File_Names;

bool build_object(Cmd *cmd, const char *src_path, const char *o_path) {
  nob_cc(cmd);
  nob_cc_my_flags(cmd);
  nob_cc_include_path(cmd, SRC_FOLDER);
  nob_cc_include_path(cmd, THIRD_PARTY_FOLDER);
  nob_cc_inputs(cmd, src_path);
  nob_cc_no_link(cmd);
  nob_cc_output(cmd, o_path);

  return cmd_run_sync_and_reset(cmd);
}

bool build_all_src_files(Cmd *cmd, const char *src_path, const char *build_path,
                         File_Names *filenames) {
  File_Paths src_files = {0};
  if (!read_entire_dir(src_path, &src_files))
    return false;

  for (int i = 0; i < src_files.count; i++) {
    const char *file = src_files.items[i];
    Nob_String_View file_sv = nob_sv_from_cstr(file);
    if (!nob_sv_end_with(file_sv, ".c")) {
      continue;
    }

    Nob_String_View filename = file_sv;
    filename.count -= 2;
    nob_da_append(filenames, filename);

    Nob_String_Builder src_sb = {0};
    nob_sb_append_cstr(&src_sb, src_path);
    nob_sb_append_cstr(&src_sb, file);
    nob_sb_append_null(&src_sb);

    Nob_String_Builder out_sb = {0};
    nob_sb_append_cstr(&out_sb, build_path);
    nob_sb_appendf(&out_sb, SV_Fmt ".o", SV_Arg(filename));
    nob_sb_append_null(&out_sb);

    if (!build_object(cmd, src_sb.items, out_sb.items))
      return false;

    nob_sb_free(src_sb);
    nob_sb_free(out_sb);
  }
  return true;
}

int main(int argc, char **argv) {
  NOB_GO_REBUILD_URSELF(argc, argv);

  if (!mkdir_if_not_exists(BUILD_FOLDER))
    return 1;

  Cmd cmd = {0};

  File_Names filenames = {0};
  if (!build_all_src_files(&cmd, SRC_FOLDER, BUILD_FOLDER, &filenames))
    return 1;

  nob_cc(&cmd);
  nob_cc_my_flags(&cmd);
  nob_cc_include_path(&cmd, SRC_FOLDER);

  for (int i = 0; i < filenames.count; i++) {
    Nob_String_View name = filenames.items[i];
    Nob_String_Builder input_sb = {0};
    nob_sb_append_cstr(&input_sb, BUILD_FOLDER);
    nob_sb_append_buf(&input_sb, name.data, name.count);
    nob_sb_appendf(&input_sb, ".o");
    nob_sb_append_null(&input_sb);

    nob_cmd_append(&cmd, input_sb.items);
  }

  nob_cmd_append(&cmd, "-lm", "-lasound", "-lGL", "-lX11", "-lXcursor", "-lXi");
  nob_cc_output(&cmd, BUILD_FOLDER "crynth");

  if (!nob_cmd_run(&cmd))
    return 1;
  return 0;
}
