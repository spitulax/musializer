#define MUSIALIZER_TARGET_NAME "linux"

bool build_musializer(void)
{
    bool result = true;
    Nob_Cmd cmd = {0};
    Nob_Procs procs = {0};

#ifdef MUSIALIZER_HOTRELOAD
    // TODO: add a way to replace `cc` with something else GCC compatible on POSIX
    // Like `clang` for instance
    nob_cmd_append(&cmd, "cc",
        "-Wall", "-Wextra", "-ggdb",
        "-I.", "-I./raylib/raylib-"RAYLIB_VERSION"/src/",
        "-fPIC", "-shared",
        "-o", "./build/libplug.so",
        "./src/plug.c", "./src/ffmpeg_linux.c",
        nob_temp_sprintf("-L./build/raylib/%s", MUSIALIZER_TARGET_NAME), "-l:libraylib.so",
        "-lm", "-ldl", "-lpthread");
    nob_da_append(&procs, nob_cmd_run_async_and_reset(&cmd));

    nob_cmd_append(&cmd, "cc",
        "-Wall", "-Wextra", "-ggdb",
        "-I.", "-I./raylib/raylib-"RAYLIB_VERSION"/src/",
        "-o", "./build/musializer",
        "./src/musializer.c", "./src/hotreload_posix.c",
        "-Wl,-rpath=./build/",
        "-Wl,-rpath=./",
        nob_temp_sprintf("-Wl,-rpath=./build/raylib/%s", MUSIALIZER_TARGET_NAME),
        // NOTE: just in case somebody wants to run musializer from within the ./build/ folder
        nob_temp_sprintf("-Wl,-rpath=./raylib/%s", MUSIALIZER_TARGET_NAME),
        nob_temp_sprintf("-L./build/raylib/%s", MUSIALIZER_TARGET_NAME),
        "-l:libraylib.so", "-lm", "-ldl", "-lpthread");
    nob_da_append(&procs, nob_cmd_run_async_and_reset(&cmd));

    if (!nob_procs_wait_and_reset(&procs)) nob_return_defer(false);
#else
    nob_cmd_append(&cmd, "cc",
        "-Wall", "-Wextra", "-ggdb",
        "-I.",
        "-I./raylib/raylib-"RAYLIB_VERSION"/src/",
        "-o", "./build/musializer",
        "./src/plug.c", "./src/ffmpeg_linux.c", "./src/musializer.c",
        nob_temp_sprintf("-L./build/raylib/%s", MUSIALIZER_TARGET_NAME), "-l:libraylib.a",
        "-lm", "-ldl", "-lpthread");
    if (!nob_cmd_run_sync_and_reset(&cmd)) nob_return_defer(false);
#endif // MUSIALIZER_HOTRELOAD

defer:
    nob_cmd_free(cmd);
    nob_da_free(procs);
    return result;
}

bool build_raylib(void)
{
    bool result = true;
    Nob_Cmd cmd = {0};
    Nob_File_Paths object_files = {0};

    if (!nob_mkdir_if_not_exists("./build/raylib")) {
        nob_return_defer(false);
    }

    Nob_Procs procs = {0};

    const char *build_path = nob_temp_sprintf("./build/raylib/%s", MUSIALIZER_TARGET_NAME);

    if (!nob_mkdir_if_not_exists(build_path)) {
        nob_return_defer(false);
    }

    for (size_t i = 0; i < NOB_ARRAY_LEN(raylib_modules); ++i) {
        const char *input_path = nob_temp_sprintf("./raylib/raylib-"RAYLIB_VERSION"/src/%s.c", raylib_modules[i]);
        const char *output_path = nob_temp_sprintf("%s/%s.o", build_path, raylib_modules[i]);
        output_path = nob_temp_sprintf("%s/%s.o", build_path, raylib_modules[i]);

        nob_da_append(&object_files, output_path);

        if (nob_needs_rebuild(output_path, &input_path, 1)) {
            nob_cmd_append(&cmd, "cc",
                "-ggdb", "-DPLATFORM_DESKTOP", "-fPIC", "-DSUPPORT_FILEFORMAT_FLAC=1",
                "-I./raylib/raylib-"RAYLIB_VERSION"/src/external/glfw/include",
                "-c", input_path,
                "-o", output_path);
            nob_da_append(&procs, nob_cmd_run_async_and_reset(&cmd));
        }
    }

    if (!nob_procs_wait_and_reset(&procs)) nob_return_defer(false);

#ifndef MUSIALIZER_HOTRELOAD
    const char *libraylib_path = nob_temp_sprintf("%s/libraylib.a", build_path);

    if (nob_needs_rebuild(libraylib_path, object_files.items, object_files.count)) {
        nob_cmd_append(&cmd, "ar", "-crs", libraylib_path);
        for (size_t i = 0; i < NOB_ARRAY_LEN(raylib_modules); ++i) {
            const char *input_path = nob_temp_sprintf("%s/%s.o", build_path, raylib_modules[i]);
            nob_cmd_append(&cmd, input_path);
        }
        if (!nob_cmd_run_sync_and_reset(&cmd)) nob_return_defer(false);
    }
#else
    const char *libraylib_path = nob_temp_sprintf("%s/libraylib.so", build_path);

    if (nob_needs_rebuild(libraylib_path, object_files.items, object_files.count)) {
        nob_cmd_append(&cmd, "cc", "-shared", "-o", libraylib_path);
        for (size_t i = 0; i < NOB_ARRAY_LEN(raylib_modules); ++i) {
            const char *input_path = nob_temp_sprintf("%s/%s.o", build_path, raylib_modules[i]);
            nob_cmd_append(&cmd, input_path);
        }
        if (!nob_cmd_run_sync_and_reset(&cmd)) nob_return_defer(false);
    }
#endif // MUSIALIZER_HOTRELOAD

defer:
    nob_cmd_free(cmd);
    nob_da_free(object_files);
    return result;
}

bool build_dist()
{
#ifdef MUSIALIZER_HOTRELOAD
    nob_log(NOB_ERROR, "We do not ship with hotreload enabled");
    return false;
#else
    if (!nob_mkdir_if_not_exists("./musializer-linux-x86_64/")) return false;
    if (!nob_copy_file("./build/musializer", "./musializer-linux-x86_64/musializer")) return false;
    if (!nob_copy_directory_recursively("./resources/", "./musializer-linux-x86_64/resources/")) return false;
    // TODO: should we pack ffmpeg with Linux build?
    // There are some static executables for Linux
    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, "tar", "fvcz", "./musializer-linux-x86_64.tar.gz", "./musializer-linux-x86_64");
    bool ok = nob_cmd_run_sync_and_reset(&cmd);
    nob_cmd_free(cmd);
    if (!ok) return false;

    return true;
#endif // MUSIALIZER_HOTRELOAD
}
