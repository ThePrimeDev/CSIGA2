if(NOT DEFINED PROD_DIR OR NOT DEFINED TARGET_FILE OR NOT DEFINED ASSETS_DIR)
  message(FATAL_ERROR "PROD_DIR, TARGET_FILE, and ASSETS_DIR must be set")
endif()

file(MAKE_DIRECTORY "${PROD_DIR}")

get_filename_component(target_name "${TARGET_FILE}" NAME)
file(COPY "${TARGET_FILE}" DESTINATION "${PROD_DIR}")
file(COPY "${ASSETS_DIR}" DESTINATION "${PROD_DIR}")

# Bundle ALL runtime dependencies (including glibc, libstdc++, etc.)
# so the prod package is fully self-contained across distros.
file(GET_RUNTIME_DEPENDENCIES
  EXECUTABLES "${TARGET_FILE}"
  RESOLVED_DEPENDENCIES_VAR deps
  UNRESOLVED_DEPENDENCIES_VAR deps_unresolved
  POST_EXCLUDE_REGEXES
    "linux-vdso.*"
)

# Helper: copy a library (resolving symlinks) into PROD_DIR
macro(copy_lib lib)
  if(IS_SYMLINK "${lib}")
    get_filename_component(_real "${lib}" REALPATH)
    get_filename_component(_name "${lib}" NAME)
    get_filename_component(_real_name "${_real}" NAME)
    file(COPY "${_real}" DESTINATION "${PROD_DIR}")
    if(NOT _name STREQUAL _real_name)
      file(CREATE_LINK "${_real_name}" "${PROD_DIR}/${_name}" SYMBOLIC)
    endif()
  else()
    file(COPY "${lib}" DESTINATION "${PROD_DIR}")
  endif()
endmacro()

foreach(dep IN LISTS deps)
  copy_lib("${dep}")
endforeach()

# Also bundle the dynamic linker (ld-linux) so we can use it in run.sh
# to guarantee a version-matched glibc + ld-linux pair.
set(ld_linux "")
foreach(candidate
    "/lib64/ld-linux-x86-64.so.2"
    "/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2"
    "/usr/lib64/ld-linux-x86-64.so.2"
    "/usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2")
  if(EXISTS "${candidate}")
    set(ld_linux "${candidate}")
    break()
  endif()
endforeach()
if(ld_linux)
  copy_lib("${ld_linux}")
  message(STATUS "Bundled dynamic linker: ${ld_linux}")
else()
  message(WARNING "Could not find ld-linux-x86-64.so.2 to bundle")
endif()

# patchelf RPATH so the binary can find libs next to itself
set(patched_binary "${PROD_DIR}/${target_name}")
execute_process(
  COMMAND patchelf --set-rpath "\$ORIGIN" "${patched_binary}"
  RESULT_VARIABLE patchelf_result
  OUTPUT_QUIET
  ERROR_QUIET
)
if(NOT patchelf_result EQUAL 0)
  message(WARNING "patchelf not found or failed; RUNPATH may not point to $ORIGIN")
endif()

# Generate run.sh that uses the BUNDLED ld-linux so the bundled glibc
# is always used, avoiding version mismatches on the target system.
set(launcher "${PROD_DIR}/run.sh")
file(WRITE "${launcher}"
"#!/bin/sh
DIR=\"$(cd \"$(dirname \"$0\")\" && pwd)\"
exec \"$DIR/ld-linux-x86-64.so.2\" --library-path \"$DIR\" \"$DIR/${target_name}\" \"$@\"
")
file(CHMOD "${launcher}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

if(deps_unresolved)
  message(WARNING "Unresolved dependencies: ${deps_unresolved}")
endif()
