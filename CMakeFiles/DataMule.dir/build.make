# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.5

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/debian/executables/PPL/Data_Mule

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/debian/executables/PPL/Data_Mule

# Include any dependencies generated for this target.
include CMakeFiles/DataMule.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/DataMule.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/DataMule.dir/flags.make

CMakeFiles/DataMule.dir/src/main.c.o: CMakeFiles/DataMule.dir/flags.make
CMakeFiles/DataMule.dir/src/main.c.o: src/main.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/debian/executables/PPL/Data_Mule/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/DataMule.dir/src/main.c.o"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/DataMule.dir/src/main.c.o   -c /home/debian/executables/PPL/Data_Mule/src/main.c

CMakeFiles/DataMule.dir/src/main.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/DataMule.dir/src/main.c.i"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/debian/executables/PPL/Data_Mule/src/main.c > CMakeFiles/DataMule.dir/src/main.c.i

CMakeFiles/DataMule.dir/src/main.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/DataMule.dir/src/main.c.s"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/debian/executables/PPL/Data_Mule/src/main.c -o CMakeFiles/DataMule.dir/src/main.c.s

CMakeFiles/DataMule.dir/src/main.c.o.requires:

.PHONY : CMakeFiles/DataMule.dir/src/main.c.o.requires

CMakeFiles/DataMule.dir/src/main.c.o.provides: CMakeFiles/DataMule.dir/src/main.c.o.requires
	$(MAKE) -f CMakeFiles/DataMule.dir/build.make CMakeFiles/DataMule.dir/src/main.c.o.provides.build
.PHONY : CMakeFiles/DataMule.dir/src/main.c.o.provides

CMakeFiles/DataMule.dir/src/main.c.o.provides.build: CMakeFiles/DataMule.dir/src/main.c.o


CMakeFiles/DataMule.dir/src/CC1200_lib.c.o: CMakeFiles/DataMule.dir/flags.make
CMakeFiles/DataMule.dir/src/CC1200_lib.c.o: src/CC1200_lib.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/debian/executables/PPL/Data_Mule/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/DataMule.dir/src/CC1200_lib.c.o"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/DataMule.dir/src/CC1200_lib.c.o   -c /home/debian/executables/PPL/Data_Mule/src/CC1200_lib.c

CMakeFiles/DataMule.dir/src/CC1200_lib.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/DataMule.dir/src/CC1200_lib.c.i"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/debian/executables/PPL/Data_Mule/src/CC1200_lib.c > CMakeFiles/DataMule.dir/src/CC1200_lib.c.i

CMakeFiles/DataMule.dir/src/CC1200_lib.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/DataMule.dir/src/CC1200_lib.c.s"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/debian/executables/PPL/Data_Mule/src/CC1200_lib.c -o CMakeFiles/DataMule.dir/src/CC1200_lib.c.s

CMakeFiles/DataMule.dir/src/CC1200_lib.c.o.requires:

.PHONY : CMakeFiles/DataMule.dir/src/CC1200_lib.c.o.requires

CMakeFiles/DataMule.dir/src/CC1200_lib.c.o.provides: CMakeFiles/DataMule.dir/src/CC1200_lib.c.o.requires
	$(MAKE) -f CMakeFiles/DataMule.dir/build.make CMakeFiles/DataMule.dir/src/CC1200_lib.c.o.provides.build
.PHONY : CMakeFiles/DataMule.dir/src/CC1200_lib.c.o.provides

CMakeFiles/DataMule.dir/src/CC1200_lib.c.o.provides.build: CMakeFiles/DataMule.dir/src/CC1200_lib.c.o


# Object files for target DataMule
DataMule_OBJECTS = \
"CMakeFiles/DataMule.dir/src/main.c.o" \
"CMakeFiles/DataMule.dir/src/CC1200_lib.c.o"

# External object files for target DataMule
DataMule_EXTERNAL_OBJECTS =

DataMule: CMakeFiles/DataMule.dir/src/main.c.o
DataMule: CMakeFiles/DataMule.dir/src/CC1200_lib.c.o
DataMule: CMakeFiles/DataMule.dir/build.make
DataMule: CMakeFiles/DataMule.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/debian/executables/PPL/Data_Mule/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking C executable DataMule"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/DataMule.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/DataMule.dir/build: DataMule

.PHONY : CMakeFiles/DataMule.dir/build

CMakeFiles/DataMule.dir/requires: CMakeFiles/DataMule.dir/src/main.c.o.requires
CMakeFiles/DataMule.dir/requires: CMakeFiles/DataMule.dir/src/CC1200_lib.c.o.requires

.PHONY : CMakeFiles/DataMule.dir/requires

CMakeFiles/DataMule.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/DataMule.dir/cmake_clean.cmake
.PHONY : CMakeFiles/DataMule.dir/clean

CMakeFiles/DataMule.dir/depend:
	cd /home/debian/executables/PPL/Data_Mule && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/debian/executables/PPL/Data_Mule /home/debian/executables/PPL/Data_Mule /home/debian/executables/PPL/Data_Mule /home/debian/executables/PPL/Data_Mule /home/debian/executables/PPL/Data_Mule/CMakeFiles/DataMule.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/DataMule.dir/depend
