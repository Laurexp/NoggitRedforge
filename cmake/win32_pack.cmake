# This file is part of Noggit3, licensed under GNU General Public License (version 3).

install(TARGETS noggit DESTINATION .)
install(DIRECTORY bin/shaders DESTINATION .)
install(DIRECTORY bin/fonts DESTINATION .)
install(DIRECTORY dist/themes DESTINATION .)
install(DIRECTORY dist/definitions DESTINATION .)
install(DIRECTORY dist/noggit-definitions DESTINATION .)
install(DIRECTORY dist/listfile DESTINATION .)
install(FILES bin/noggit_template.conf bin/freetype6.dll bin/StormLib.dll bin/glew32.dll bin/zlib1.dll DESTINATION . OPTIONAL)

install(CODE "execute_process(COMMAND \"${_qt_bin_dir}/windeployqt.exe\" --no-compiler-runtime \"\${CMAKE_INSTALL_PREFIX}/noggit.exe\")")

set(CPACK_GENERATOR "ZIP;NSIS")
# \todo Re-add proper version, if we ever use cpack again.
SET(CPACK_BUNDLE_NAME "Noggit_3")
SET(CPACK_PACKAGE_FILE_NAME "Noggit")
SET(CPACK_PACKAGE_VERSION "3")

include(CPack)
