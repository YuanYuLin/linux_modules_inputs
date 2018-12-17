import ops
import iopc

pkg_path = ""
output_dir = ""
crosscc = ""
build_arch = ""
kmod_dir = ""

def set_global(args):
    global pkg_path
    global output_dir 
    global crosscc
    global build_arch
    global kmod_dir
    pkg_path = args["pkg_path"]
    output_dir = args["output_path"]
    crosscc = ops.getEnv("CROSS_COMPILE")
    build_arch = ops.getEnv("ARCH")
    kmod_dir = ops.getEnv("LINUXKERNELMODULEROOT")

def MAIN_ENV(args):
    set_global(args)

    return False

def MAIN_EXTRACT(args):
    set_global(args)

    ops.copyto(ops.path_join(pkg_path, "src/."), output_dir)

    return True

def MAIN_PATCH(args, patch_group_name):
    set_global(args)
    for patch in iopc.get_patch_list(pkg_path, patch_group_name):
        if iopc.apply_patch(build_dir, patch):
            continue
        else:
            sys.exit(1)

    return True

def MAIN_CONFIGURE(args):
    set_global(args)

    return True

def MAIN_BUILD(args):
    set_global(args)

    extra_conf = []
    extra_conf.append("CROSS_COMPILE=" + crosscc)
    extra_conf.append("ARCH=" + build_arch)
    extra_conf.append("KERNEL_MODULE_DIR=" + output_dir)
    iopc.make(output_dir, extra_conf)

    return False

def MAIN_INSTALL(args):
    set_global(args)

    ops.copyto(ops.path_join(output_dir, "iopc_inputs.ko"), kmod_dir)

    return False

def MAIN_SDKENV(args):
    set_global(args)

    return False

def MAIN_CLEAN_BUILD(args):
    set_global(args)

    return False

def MAIN(args):
    print "iopclauncher"

