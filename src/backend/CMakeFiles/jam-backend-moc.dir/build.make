# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 2.4

#=============================================================================
# Special targets provided by cmake.

# Suppress display of executed commands.
$(VERBOSE).SILENT:

# Disable implicit rules so canoncical targets will work.
.SUFFIXES:

.SUFFIXES: .hpux_make_needs_suffix_list

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# The program to use to edit the cache.
CMAKE_EDIT_COMMAND = /usr/bin/ccmake

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/jb/devel/cc/jam

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/jb/devel/cc/jam

# Include any dependencies generated for this target.
include src/backend/CMakeFiles/jam-backend-moc.dir/depend.make

# Include the progress variables for this target.
include src/backend/CMakeFiles/jam-backend-moc.dir/progress.make

# Include the compile flags for this target's objects.
include src/backend/CMakeFiles/jam-backend-moc.dir/flags.make

src/backend/CMakeFiles/jam-backend-moc.dir/depend.make.mark: src/backend/CMakeFiles/jam-backend-moc.dir/flags.make
src/backend/CMakeFiles/jam-backend-moc.dir/depend.make.mark: src/backend/moc_vdrrequest.cxx

src/backend/CMakeFiles/jam-backend-moc.dir/moc_vdrrequest.o: src/backend/CMakeFiles/jam-backend-moc.dir/flags.make
src/backend/CMakeFiles/jam-backend-moc.dir/moc_vdrrequest.o: src/backend/moc_vdrrequest.cxx
	$(CMAKE_COMMAND) -E cmake_progress_report /home/jb/devel/cc/jam/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object src/backend/CMakeFiles/jam-backend-moc.dir/moc_vdrrequest.o"
	/usr/bin/c++   $(CXX_FLAGS) -o src/backend/CMakeFiles/jam-backend-moc.dir/moc_vdrrequest.o -c /home/jb/devel/cc/jam/src/backend/moc_vdrrequest.cxx

src/backend/CMakeFiles/jam-backend-moc.dir/moc_vdrrequest.o.requires:

src/backend/CMakeFiles/jam-backend-moc.dir/moc_vdrrequest.o.provides: src/backend/CMakeFiles/jam-backend-moc.dir/moc_vdrrequest.o.requires
	$(MAKE) -f src/backend/CMakeFiles/jam-backend-moc.dir/build.make src/backend/CMakeFiles/jam-backend-moc.dir/moc_vdrrequest.o.provides.build

src/backend/CMakeFiles/jam-backend-moc.dir/moc_vdrrequest.o.provides.build: src/backend/CMakeFiles/jam-backend-moc.dir/moc_vdrrequest.o

src/backend/moc_vdrrequest.cxx: /usr/bin/moc
src/backend/moc_vdrrequest.cxx: src/backend/vdrrequest.h
	$(CMAKE_COMMAND) -E cmake_progress_report /home/jb/devel/cc/jam/CMakeFiles $(CMAKE_PROGRESS_2)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold "QT Wrapped File"
	cd /home/jb/devel/cc/jam/src/backend && /usr/bin/moc -o /home/jb/devel/cc/jam/src/backend/moc_vdrrequest.cxx /home/jb/devel/cc/jam/src/backend/vdrrequest.h

src/backend/CMakeFiles/jam-backend-moc.dir/depend: src/backend/CMakeFiles/jam-backend-moc.dir/depend.make.mark

src/backend/CMakeFiles/jam-backend-moc.dir/depend.make.mark:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --magenta --bold "Scanning dependencies of target jam-backend-moc"
	cd /home/jb/devel/cc/jam && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/jb/devel/cc/jam /home/jb/devel/cc/jam/src/backend /home/jb/devel/cc/jam /home/jb/devel/cc/jam/src/backend /home/jb/devel/cc/jam/src/backend/CMakeFiles/jam-backend-moc.dir/DependInfo.cmake

# Object files for target jam-backend-moc
jam__backend__moc_OBJECTS = \
"CMakeFiles/jam-backend-moc.dir/moc_vdrrequest.o"

# External object files for target jam-backend-moc
jam__backend__moc_EXTERNAL_OBJECTS =

src/backend/libjam-backend-moc.a: src/backend/CMakeFiles/jam-backend-moc.dir/moc_vdrrequest.o
src/backend/libjam-backend-moc.a: src/backend/CMakeFiles/jam-backend-moc.dir/build.make
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking CXX static library libjam-backend-moc.a"
	cd /home/jb/devel/cc/jam/src/backend && $(CMAKE_COMMAND) -P CMakeFiles/jam-backend-moc.dir/cmake_clean_target.cmake
	cd /home/jb/devel/cc/jam/src/backend && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/jam-backend-moc.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/backend/CMakeFiles/jam-backend-moc.dir/build: src/backend/libjam-backend-moc.a
src/backend/CMakeFiles/jam-backend-moc.dir/build: src/backend/moc_vdrrequest.cxx

src/backend/CMakeFiles/jam-backend-moc.dir/requires: src/backend/CMakeFiles/jam-backend-moc.dir/moc_vdrrequest.o.requires

src/backend/CMakeFiles/jam-backend-moc.dir/clean:
	cd /home/jb/devel/cc/jam/src/backend && $(CMAKE_COMMAND) -P CMakeFiles/jam-backend-moc.dir/cmake_clean.cmake

