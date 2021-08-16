# generate all in one portablegl.h from component libraries/files

import sys, os, glob, argparse

mangle_types = """
#ifdef MANGLE_TYPES
#define vec2 glinternal_vec2
#define vec3 glinternal_vec3
#define vec4 glinternal_vec4
#define dvec2 glinternal_dvec2
#define dvec3 glinternal_dvec3
#define dvec4 glinternal_dvec4
#define ivec2 glinternal_ivec2
#define ivec3 glinternal_ivec3
#define ivec4 glinternal_ivec4
#define uvec2 glinternal_uvec2
#define uvec3 glinternal_uvec3
#define uvec4 glinternal_uvec4
#define mat2 glinternal_mat2
#define mat3 glinternal_mat3
#define mat4 glinternal_mat4
#define Color glinternal_Color
#define Line glinternal_Line
#define Plane glinternal_Plane
#endif
"""

unmangle_types = """
#ifdef MANGLE_TYPES
#undef vec2
#undef vec3
#undef vec4
#undef dvec2
#undef dvec3
#undef dvec4
#undef ivec2
#undef ivec3
#undef ivec4
#undef uvec2
#undef uvec3
#undef uvec4
#undef mat2
#undef mat3
#undef mat4
#undef Color
#undef Line
#undef Plane
#endif
"""

open_header = """
#ifndef GL_H
#define GL_H


#ifdef __cplusplus
extern "C" {
#endif

"""

close_header = """
#ifdef __cplusplus
}
#endif

// end GL_H
#endif

"""

prefix_macro = """

#ifndef PGL_PREFIX
#define PGL_PREFIX(x) x
#endif

"""



def cvector_impl(type_name):
    s = "#define CVECTOR_" + type_name + "_IMPLEMENTATION\n"
    s += open("cvector_" + type_name + ".h").read()
    return s


def get_gl_funcs():
    functions = [l.rstrip() for l in open("gl_function_list.c").readlines() if l.startswith('gl')]
    return functions




if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate the single-file-header portablegl.h from component source files")
    parser.add_argument("-u", "--unsafe", help="Generate with unsafe gl implementation (no error checking)", action='store_true')
    parser.add_argument("-p", "--prefix_macro", help="Wrap all gl functions in a macro to enable prefix/namespacing", action='store_true')
    #parser.add_argument("-m", "--macros", help="Use macros to include vectors", action='store_true')
    args = parser.parse_args()
    print(args, file=sys.stderr)

    gl_impl = ''
    gl_prototypes = open("gl_prototypes.h").read()

    if not args.unsafe:
        gl_h = open("portablegl.h", "w")
        gl_impl = open("gl_impl.c").read()
    else:
        gl_h = open("portablegl_unsafe.h", "w")
        gl_impl = open("gl_impl_unsafe.c").read()

    if args.prefix_macro:
        for func in get_gl_funcs():
            # Really hacky ... I could just move pglCreateProgram (and other non-standard funcs) to pgl_ext.c/h
            # Or I could just bite the bullet and use a regex...
            gl_impl = gl_impl.replace(" "+func+"(", " PGL_PREFIX("+func+")(")
            gl_impl = gl_impl.replace("\t"+func+"(", "\tPGL_PREFIX("+func+")(")
            gl_prototypes = gl_prototypes.replace(" "+func+"(", " PGL_PREFIX("+func+")(")

        gl_prototypes = prefix_macro + gl_prototypes


    gl_h.write("/*\n")

    gl_h.write(open("header_docs.txt").read())

    gl_h.write("*/\n")

    gl_h.write(mangle_types)
    gl_h.write(open_header)

    gl_h.write(open("crsw_math.h").read())

    # we actually use this for output_buf in gl_types
    gl_h.write(open("cvector_float.h").read())

    # maybe an option to add vectors for commonly used types
    # vec2/3/4, ivec3 etc.?
    #gl_h.write(open("cvector_vec3.h").read())
    #gl_h.write(open("cvector_vec4.h").read())

    gl_h.write(open("gl_types.h").read())

    # could put these as macros at top of glcontext.h
    gl_h.write(open("cvector_glVertex_Array.h").read())
    gl_h.write(open("cvector_glBuffer.h").read())
    gl_h.write(open("cvector_glTexture.h").read())
    gl_h.write(open("cvector_glProgram.h").read())
    gl_h.write(open("cvector_glVertex.h").read())

    gl_h.write(open("glcontext.h").read())

    gl_h.write(open("gl_glsl.h").read())

    gl_h.write(gl_prototypes)
    gl_h.write(open("pgl_ext.h").read())


    gl_h.write(close_header)


    # part 2
    gl_h.write("#ifdef PORTABLEGL_IMPLEMENTATION\n\n")

    gl_h.write(open("crsw_math.c").read())

    # handle #define'ing CVECTOR_TYPE_IMPLEMENTATION for each ...
    # maybe I should stick to using cvector_macro.h and use the macros
    gl_h.write(cvector_impl("glVertex_Array"))
    gl_h.write(cvector_impl("glBuffer"))
    gl_h.write(cvector_impl("glTexture"))
    gl_h.write(cvector_impl("glProgram"))
    gl_h.write(cvector_impl("glVertex"))

    gl_h.write(cvector_impl("float"))

    #gl_h.write(cvector_impl("vec3"))
    #gl_h.write(cvector_impl("vec4"))


    gl_h.write(open("gl_internal.c").read())

    gl_h.write(gl_impl)

    gl_h.write(open("gl_glsl.c").read())
    gl_h.write(open("pgl_ext.c").read())


    # This prevents it from being implemented twice if you include it again in the same
    # file (usually indirectly, ie including the header for some OpenGL helper functions that
    # in turn have to include PortableGL).  Otherwise you'd have to be much more careful about
    # the order and dependencies of inclusions which is a pain
    gl_h.write("#undef PORTABLEGL_IMPLEMENTATION\n")
    gl_h.write("#undef CVECTOR_float_IMPLEMENTATION\n")
    gl_h.write("#endif\n")


    gl_h.write(unmangle_types)



