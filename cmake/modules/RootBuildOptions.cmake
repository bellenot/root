# Copyright (C) 1995-2021, Rene Brun and Fons Rademakers.
# All rights reserved.
#
# For the licensing terms see $ROOTSYS/LICENSE.
# For the list of contributors see $ROOTSYS/README/CREDITS.

set(root_build_options)

#---------------------------------------------------------------------------------------------------
#---ROOT_BUILD_OPTION( name defvalue [description] )
#---------------------------------------------------------------------------------------------------
function(ROOT_BUILD_OPTION opt defvalue)
  if(ARGN)
    set(description ${ARGN})
  else()
    set(description " ")
  endif()
  set(${opt}_defvalue    ${defvalue} PARENT_SCOPE)
  set(${opt}_description ${description} PARENT_SCOPE)
  set(root_build_options  ${root_build_options} ${opt} PARENT_SCOPE )
endfunction()

#---------------------------------------------------------------------------------------------------
#---ROOT_APPLY_OPTIONS()
#---------------------------------------------------------------------------------------------------
function(ROOT_APPLY_OPTIONS)
  foreach(opt ${root_build_options})
     option(${opt} "${${opt}_description}" ${${opt}_defvalue})
  endforeach()
endfunction()

#---------------------------------------------------------------------------------------------------
#---ROOT_GET_OPTIONS(result ENABLED)
#---------------------------------------------------------------------------------------------------
function(ROOT_GET_OPTIONS result)
  CMAKE_PARSE_ARGUMENTS(ARG "ENABLED" "" "" ${ARGN})
  set(enabled)
  foreach(opt ${root_build_options})
    if(ARG_ENABLED)
      if(${opt})
        set(enabled "${enabled} ${opt}")
      endif()
    else()
      set(enabled "${enabled} ${opt}")
    endif()
  endforeach()
  set(${result} "${enabled}" PARENT_SCOPE)
endfunction()

#---------------------------------------------------------------------------------------------------
#---ROOT_SHOW_ENABLED_OPTIONS()
#---------------------------------------------------------------------------------------------------
function(ROOT_SHOW_ENABLED_OPTIONS)
  set(enabled_opts)
  ROOT_GET_OPTIONS(enabled_opts ENABLED)
  foreach(opt ${enabled_opts})
    message(STATUS "Enabled support for: ${opt}")
  endforeach()
endfunction()

#---------------------------------------------------------------------------------------------------
#---ROOT_WRITE_OPTIONS(file )
#---------------------------------------------------------------------------------------------------
function(ROOT_WRITE_OPTIONS file)
  file(WRITE ${file} "#---Options enabled for the build of ROOT-----------------------------------------------\n")
  foreach(opt ${root_build_options})
    if(${opt})
      file(APPEND ${file} "set(${opt} ON)\n")
    else()
      file(APPEND ${file} "set(${opt} OFF)\n")
    endif()
  endforeach()
endfunction()

#--------------------------------------------------------------------------------------------------
#---Full list of options with their descriptions and default values
#   The default value can be changed as many times as we wish before calling ROOT_APPLY_OPTIONS()
#--------------------------------------------------------------------------------------------------

ROOT_BUILD_OPTION(arrow OFF "Enable support for Apache Arrow")
ROOT_BUILD_OPTION(asimage ON "Enable support for image processing via libAfterImage")
ROOT_BUILD_OPTION(asimage_tiff ON "Support TIFF in image processing (requires libtiff)")
ROOT_BUILD_OPTION(asserts OFF "Enable asserts (defaults to ON for CMAKE_BUILD_TYPE=Debug and/or dev=ON)")
ROOT_BUILD_OPTION(builtin_cfitsio OFF "Build CFITSIO internally (requires network)")
ROOT_BUILD_OPTION(builtin_clang ON "Build bundled copy of Clang")
ROOT_BUILD_OPTION(builtin_cling ON "Build bundled copy of Cling. Only build with an external cling if you know what you are doing: associating ROOT commits with cling commits is tricky.")
MARK_AS_ADVANCED(builtin_cling)
ROOT_BUILD_OPTION(builtin_cppzmq OFF "Use ZeroMQ C++ bindings installed by ROOT (requires network)")
ROOT_BUILD_OPTION(builtin_davix OFF "Build Davix internally (requires network)")
ROOT_BUILD_OPTION(builtin_fftw3 OFF "Build FFTW3 internally (requires network)")
ROOT_BUILD_OPTION(builtin_freetype OFF "Build bundled copy of freetype")
ROOT_BUILD_OPTION(builtin_ftgl OFF "Build bundled copy of FTGL")
ROOT_BUILD_OPTION(builtin_gif OFF "Build bundled copy of libgif")
ROOT_BUILD_OPTION(builtin_gl2ps OFF "Build bundled copy of gl2ps")
ROOT_BUILD_OPTION(builtin_glew OFF "Build bundled copy of GLEW")
ROOT_BUILD_OPTION(builtin_gsl OFF "Build GSL internally (requires network)")
ROOT_BUILD_OPTION(builtin_gtest OFF "Build googletest internally (requires network)")
ROOT_BUILD_OPTION(builtin_jpeg OFF "Build bundled copy of libjpeg")
ROOT_BUILD_OPTION(builtin_llvm ON "Build bundled copy of LLVM")
ROOT_BUILD_OPTION(builtin_lz4 OFF "Build bundled copy of lz4")
ROOT_BUILD_OPTION(builtin_lzma OFF "Build bundled copy of lzma")
ROOT_BUILD_OPTION(builtin_nlohmannjson OFF "Use nlohmann/json.hpp file distributed with ROOT")
ROOT_BUILD_OPTION(builtin_openssl OFF "Build OpenSSL internally (requires network)")
ROOT_BUILD_OPTION(builtin_openui5 ON "Use openui5 bundle distributed with ROOT")
ROOT_BUILD_OPTION(builtin_pcre OFF "Build bundled copy of PCRE")
ROOT_BUILD_OPTION(builtin_png OFF "Build bundled copy of libpng")
ROOT_BUILD_OPTION(builtin_tbb OFF "Build TBB internally (requires network)")
ROOT_BUILD_OPTION(builtin_unuran OFF "Build bundled copy of unuran")
ROOT_BUILD_OPTION(builtin_vc OFF "Build Vc internally (requires network)")
ROOT_BUILD_OPTION(builtin_vdt OFF "Build VDT internally (requires network)")
ROOT_BUILD_OPTION(builtin_veccore OFF "Build VecCore internally (requires network)")
ROOT_BUILD_OPTION(builtin_xrootd OFF "Build XRootD internally (requires network)")
ROOT_BUILD_OPTION(builtin_xxhash OFF "Build bundled copy of xxHash")
ROOT_BUILD_OPTION(builtin_zeromq OFF "Build ZeroMQ internally (requires network)")
ROOT_BUILD_OPTION(builtin_zlib OFF "Build bundled copy of zlib")
ROOT_BUILD_OPTION(builtin_zstd OFF "Build included libzstd, or use system libzstd")
ROOT_BUILD_OPTION(ccache OFF "Enable ccache usage for speeding up builds")
ROOT_BUILD_OPTION(cefweb OFF "Enable support for CEF (Chromium Embedded Framework) web-based display")
ROOT_BUILD_OPTION(check_connection ON "Fail the configuration step if there is no internet connection, but it's required for the build")
ROOT_BUILD_OPTION(clad ON "Build clad, the cling automatic differentiation plugin (requires network, or existing source directory indicated with -DCLAD_SOURCE_DIR=<clad_src_path>)")
ROOT_BUILD_OPTION(cocoa OFF "Use native Cocoa/Quartz graphics backend (MacOS X only)")
ROOT_BUILD_OPTION(coverage OFF "Enable compile flags for coverage testing")
ROOT_BUILD_OPTION(cuda OFF "Enable support for CUDA (requires CUDA toolkit >= 7.5)")
ROOT_BUILD_OPTION(daos OFF "Enable RNTuple support for Intel DAOS")
ROOT_BUILD_OPTION(dataframe ON "Enable ROOT RDataFrame")
ROOT_BUILD_OPTION(davix ON "Enable support for Davix (HTTP/WebDAV access)")
ROOT_BUILD_OPTION(dcache OFF "Enable support for dCache (requires libdcap from DESY)")
ROOT_BUILD_OPTION(dev OFF "Enable recommended developer compilation flags, reduce exposed includes")
ROOT_BUILD_OPTION(distcc OFF "Enable distcc usage for speeding up builds (ccache is called first if enabled)")
ROOT_BUILD_OPTION(fcgi OFF "Enable FastCGI support in HTTP server")
ROOT_BUILD_OPTION(fftw3 OFF "Enable support for FFTW3 [GPL]")
ROOT_BUILD_OPTION(fitsio ON "Enable support for reading FITS images")
ROOT_BUILD_OPTION(fortran OFF "Build Fortran components of ROOT")
ROOT_BUILD_OPTION(gdml ON "Enable support for GDML (Geometry Description Markup Language)")
ROOT_BUILD_OPTION(geom ON "Enable support for the geometry library. Disabling this will also disable Eve and gviz3d.")
ROOT_BUILD_OPTION(geombuilder OFF "Enable support for the geombuilder library")
ROOT_BUILD_OPTION(gnuinstall OFF "Perform installation following the GNU guidelines")
ROOT_BUILD_OPTION(gviz OFF "Enable support for Graphviz (graph visualization software)")
ROOT_BUILD_OPTION(http ON "Enable support for HTTP server")
ROOT_BUILD_OPTION(imt ON "Enable support for implicit multi-threading via Intel® Thread Building Blocks (TBB)")
ROOT_BUILD_OPTION(libcxx OFF "Build using libc++")
ROOT_BUILD_OPTION(llvm13_broken_tests OFF "Enable broken tests with LLVM 13 on Windows")
ROOT_BUILD_OPTION(macos_native OFF "Disable looking for libraries, includes and binaries in locations other than a native installation (MacOS only)")
ROOT_BUILD_OPTION(mathmore OFF "Build libMathMore extended math library (requires GSL) [GPL]")
ROOT_BUILD_OPTION(memory_termination OFF "Free internal ROOT memory before process termination (experimental, used for leak checking)")
ROOT_BUILD_OPTION(minuit2_mpi OFF "Enable support for MPI in Minuit2")
ROOT_BUILD_OPTION(minuit2_omp OFF "Enable support for OpenMP in Minuit2")
ROOT_BUILD_OPTION(mpi OFF "Enable support for Message Passing Interface (MPI)")
ROOT_BUILD_OPTION(opengl ON "Enable support for OpenGL (requires libGL and libGLU)")
ROOT_BUILD_OPTION(pyroot ON "Enable support for automatic Python bindings (PyROOT)")
ROOT_BUILD_OPTION(pythia8 OFF "Enable support for Pythia 8.x [GPL]")
ROOT_BUILD_OPTION(qt6web OFF "Enable support for Qt6 web-based display (requires Qt6::WebEngineCore and Qt6::WebEngineWidgets)")
ROOT_BUILD_OPTION(r OFF "Enable support for R bindings (requires R, Rcpp, and RInside)")
ROOT_BUILD_OPTION(roofit ON "Build the advanced fitting package RooFit, and RooStats for statistical tests. If xml is available, also build HistFactory.")
ROOT_BUILD_OPTION(roofit_multiprocess OFF "Build RooFit::MultiProcess and multi-process RooFit::TestStatistics classes (requires ZeroMQ >= 3.4.5 built with -DENABLE_DRAFTS and cppzmq).")
ROOT_BUILD_OPTION(root7 ON "Build ROOT 7 components of ROOT")
ROOT_BUILD_OPTION(rpath ON "Link libraries with built-in RPATH (run-time search path)")
ROOT_BUILD_OPTION(runtime_cxxmodules ON "Enable runtime support for C++ modules")
ROOT_BUILD_OPTION(shadowpw OFF "Enable support for shadow passwords")
ROOT_BUILD_OPTION(shared ON "Use shared 3rd party libraries if possible")
ROOT_BUILD_OPTION(soversion OFF "Set version number in sonames for shared libraries. Not recommended, as the pcm and rootmap files do not (yet) support versioning and always point to the non-versioned shared libraries.")
ROOT_BUILD_OPTION(spectrum ON "Enable support for TSpectrum")
ROOT_BUILD_OPTION(sqlite ON "Enable support for SQLite")
ROOT_BUILD_OPTION(ssl ON "Enable support for SSL encryption via OpenSSL")
ROOT_BUILD_OPTION(test_distrdf_dask OFF "Enable distributed RDataFrame tests that use dask")
ROOT_BUILD_OPTION(test_distrdf_pyspark OFF "Enable distributed RDataFrame tests that use pyspark")
ROOT_BUILD_OPTION(testsupport OFF "Build the ROOT::TestSupport library required to use all features of ROOT_ADD_GTEST and similar macros (requires gtest at build time)")
ROOT_BUILD_OPTION(thisroot_scripts ON "Build scripts like thisroot.{sh, fish, etc.} that set environment paths for using ROOT. Usually not needed when building ROOT for the distribution with a package manager.")
ROOT_BUILD_OPTION(tmva ON "Build TMVA multi variate analysis library")
ROOT_BUILD_OPTION(tmva-cpu ON "Build TMVA with CPU support for deep learning (requires BLAS)")
ROOT_BUILD_OPTION(tmva-cudnn ON "Enable support for cuDNN (default when CUDA is enabled)")
ROOT_BUILD_OPTION(tmva-gpu OFF "Build TMVA with GPU support for deep learning (requires CUDA)")
ROOT_BUILD_OPTION(tmva-pymva ON "Enable support for Python in TMVA (requires numpy)")
ROOT_BUILD_OPTION(tmva-rmva OFF "Enable support for R in TMVA")
ROOT_BUILD_OPTION(tmva-sofie OFF "Build TMVA with support for sofie - fast inference code generation (requires protobuf 3)")
ROOT_BUILD_OPTION(tpython ON "Build the TPython class that allows you to run Python code from C++")
ROOT_BUILD_OPTION(unfold OFF "Enable the unfold package [GPL]")
ROOT_BUILD_OPTION(unuran OFF "Enable support for UNURAN (package for generating non-uniform random numbers) [GPL]")
ROOT_BUILD_OPTION(uring OFF "Enable support for io_uring (requires liburing and Linux kernel >= 5.1)")
ROOT_BUILD_OPTION(use_gsl_cblas ON "Use the CBLAS library from GSL instead of finding a more optimized BLAS library automatically with FindBLAS (the GSL CBLAS is less performant but more portable)")
ROOT_BUILD_OPTION(vc OFF "Enable support for Vc (SIMD Vector Classes for C++)")
ROOT_BUILD_OPTION(vdt ON "Enable support for VDT (fast and vectorisable mathematical functions)")
ROOT_BUILD_OPTION(veccore OFF "Enable support for VecCore SIMD abstraction library")
ROOT_BUILD_OPTION(vecgeom OFF "Enable support for VecGeom vectorized geometry library")
ROOT_BUILD_OPTION(webgui ON "Build Web-based UI components of ROOT")
ROOT_BUILD_OPTION(win_broken_tests OFF "Enable broken tests on Windows")
ROOT_BUILD_OPTION(winrtdebug OFF "Link against the Windows debug runtime library")
ROOT_BUILD_OPTION(x11 ON "Enable support for X11/Xft")
ROOT_BUILD_OPTION(xml ON "Enable support for XML (requires libxml2)")
ROOT_BUILD_OPTION(xrootd ON "Enable support for XRootD file server and client")

option(all "Enable all optional components by default" OFF)
option(clingtest "Enable cling tests (Note: that this makes llvm/clang symbols visible in libCling)" OFF)
option(fail-on-missing "Fail at configure time if a required package cannot be found" OFF)
option(gminimal "Enable only required options by default, but include X11" OFF)
option(minimal "Enable only required options by default" OFF)
option(rootbench "Build rootbench if rootbench exists in root or if it is a sibling directory." OFF)
option(roottest "Build roottest if roottest exists in root or if it is a sibling directory." OFF)
option(testing "Enable testing with CTest" OFF)
option(asan "Build ROOT with address sanitizer instrumentation" OFF)

set(gcctoolchain "" CACHE PATH "Set path to GCC toolchain used to build llvm/clang")

if(all AND minimal)
  message(FATAL_ERROR "The 'all' and 'minimal' options are mutually exclusive")
endif()

#--- Compression algorithms in ROOT-------------------------------------------------------------
set(compression_default "zlib" CACHE STRING "Default compression algorithm (zlib (default), lz4, zstd or lzma)")
string(TOLOWER "${compression_default}" compression_default)
if("${compression_default}" MATCHES "zlib|lz4|lzma|zstd")
  message(STATUS "ROOT default compression algorithm: ${compression_default}")
else()
  message(FATAL_ERROR "Unsupported compression algorithm: ${compression_default}\n"
    "Known values are zlib, lzma, lz4, zstd (case-insensitive).")
endif()

#--- The 'all' option switches ON major options---------------------------------------------------
if(all)
 set(arrow_defvalue ON)
 set(asimage_defvalue ON)
 set(asimage_tiff_defvalue ON)
 set(cefweb_defvalue ON)
 set(clad_defvalue ON)
 set(dataframe_defvalue ON)
 set(davix_defvalue ON)
 set(dcache_defvalue ON)
 set(fftw3_defvalue ON)
 set(fitsio_defvalue ON)
 set(fortran_defvalue ON)
 set(gdml_defvalue ON)
 set(gviz_defvalue ON)
 set(http_defvalue ON)
 set(fcgi_defvalue ON)
 set(imt_defvalue ON)
 set(mathmore_defvalue ON)
 set(opengl_defvalue ON)
 set(pythia8_defvalue ON)
 set(pyroot_defvalue ON)
 set(qt6web_defvalue ON)
 set(r_defvalue ON)
 set(roofit_defvalue ON)
 set(roofit_multiprocess_defvalue ON)
 set(webgui_defvalue ON)
 set(root7_defvalue ON)
 set(shadowpw_defvalue ON)
 set(sqlite_defvalue ON)
 set(ssl_defvalue ON)
 set(tmva_defvalue ON)
 set(tmva-cpu_defvalue ON)
 set(tmva-pymva_defvalue ON)
 set(tmva-rmva_defvalue ON)
 set(unuran_defvalue ON)
 set(vc_defvalue ON)
 set(vdt_defvalue ON)
 set(veccore_defvalue ON)
 set(vecgeom_defvalue ON)
 set(x11_defvalue ON)
 set(xml_defvalue ON)
 set(xrootd_defvalue ON)
 if(CMAKE_CUDA_COMPILER)
   set(cuda_defvalue ON)
   set(tmva-gpu_defvalue ON)
 endif()
endif()

#--- The 'builtin_all' option switches ON all the built in options but GPL-------------------------------
if(builtin_all)
  set(builtin_cfitsio_defvalue ON)
  set(builtin_clang_defvalue ON)
  set(builtin_cling_defvalue ON)
  set(builtin_cppzmq_defvalue ON)
  set(builtin_davix_defvalue ON)
#  set(builtin_fftw3_defvalue ON) (GPL)
  set(builtin_freetype_defvalue ON)
  set(builtin_ftgl_defvalue ON)
  set(builtin_gif_defvalue ON)
  set(builtin_gl2ps_defvalue ON)
  set(builtin_glew_defvalue ON)
#  set(builtin_gsl_defvalue ON) (GPL)
  set(builtin_gtest_defvalue ON)
  set(builtin_jpeg_defvalue ON)
  set(builtin_llvm_defvalue ON)
  set(builtin_lz4_defvalue ON)
  set(builtin_lzma_defvalue ON)
  set(builtin_nlohmannjson_defvalue ON)
  if(APPLE)
    set(builtin_openssl_defvalue ON)
  endif()
  set(builtin_openui5_defvalue ON)
  set(builtin_pcre_defvalue ON)
  set(builtin_png_defvalue ON)
  set(builtin_tbb_defvalue ON)
#  set(builtin_unuran_defvalue ON) (GPL)
  set(builtin_vc_defvalue ON)
  set(builtin_vdt_defvalue ON)
  set(builtin_veccore_defvalue ON)
  set(builtin_xrootd_defvalue ON)
  set(builtin_xxhash_defvalue ON)
  set(builtin_zeromq_defvalue ON)
  set(builtin_zlib_defvalue ON)
  set(builtin_zstd_defvalue ON)
endif()

#---Changes in defaults due to platform-------------------------------------------------------
if(WIN32)
  set(davix_defvalue OFF)
  set(roofit_multiprocess_defvalue OFF)
  set(roottest_defvalue OFF)
  set(rpath_defvalue OFF)
  set(runtime_cxxmodules_defvalue OFF)
  set(testing_defvalue OFF)
  set(vdt_defvalue OFF)
  set(x11_defvalue OFF)
  set(xrootd_defvalue OFF)
elseif(APPLE)
  set(cocoa_defvalue ON)
  set(x11_defvalue OFF)
endif()

# builtin_openssl is only supported on macOS
if(builtin_openssl AND NOT APPLE)
    message(FATAL_ERROR ">>> Option 'builtin_openssl' is only supported on macOS.")
endif()

# MultiProcess is not possible on Windows, so fail if it is manually set:
if(roofit_multiprocess AND WIN32)
    message(FATAL_ERROR ">>> Option 'roofit_multiprocess' is not supported on Windows.")
endif()

#---Options depending of CMake Generator-------------------------------------------------------
if( CMAKE_GENERATOR STREQUAL Ninja)
   set(fortran_defvalue OFF)
endif()

#---Apply minimal or gminimal------------------------------------------------------------------
foreach(opt ${root_build_options})
  if(NOT opt MATCHES "builtin_llvm|builtin_clang|builtin_cling|shared|runtime_cxxmodules|thisroot_scripts")
    if(minimal)
      set(${opt}_defvalue OFF)
    elseif(gminimal AND NOT opt MATCHES "x11|cocoa")
      set(${opt}_defvalue OFF)
    endif()
  endif()
endforeach()

#---webgui by default always build together with root7-----------------------------------------
set(webgui_defvalue ${root7_defvalue})

#---Enable asserts for Debug builds and for the dev mode---------------------------------------
if(_BUILD_TYPE_UPPER STREQUAL DEBUG OR dev)
  set(asserts_defvalue ON)
endif()

#---Define at moment the options with the selected default values------------------------------
ROOT_APPLY_OPTIONS()

#---roottest option implies testing
if(roottest OR rootbench)
  set(testing ON CACHE BOOL "" FORCE)
endif()

#---testing implies testsupport
if(testing)
  set(testsupport ON CACHE BOOL "" FORCE)
endif()


if(unfold AND NOT xml)
  message(STATUS "Cannot enable unfold without enabling xml: unfold is disabled.")
  set(unfold OFF)
endif()

if (NOT builtin_cling)
  if (builtin_clang OR builtin_llvm)
    message(WARNING "No need to build internal llvm or clang. Consider turning builtin_clang=Off and builtin_llvm=Off")
  endif()
endif(NOT builtin_cling)

if(NOT http AND webgui)
   message(WARNING "Cannot build WebGui components without HTTP: webgui is disabled.")
   set(webgui OFF)
endif()

if(NOT webgui)
   set(qt6web OFF CACHE BOOL "Disabled because webgui not build" FORCE)
   set(cefweb OFF CACHE BOOL "Disabled because webgui not build" FORCE)
endif()

#---Removed options------------------------------------------------------------
# Please notify SPI when adding to this list
foreach(opt afdsmgrd afs alien bonjour builtin_afterimage castor chirp cxx11 cxx14 cxx17
        cxxmodules exceptions geocad gfal glite globus gsl_shared hdfs html ios jemalloc krb5
        ldap memstat minuit2 monalisa oracle proof pyroot-python2 pyroot_legacy
        pythia6 pythia6_nolink python qt qtgsi qt5web rfio ruby sapdb srp table
        tcmalloc vmc xproofd mysql odbc pgsql)
  if(${opt})
    message(FATAL_ERROR ">>> '${opt}' is no longer part of ROOT ${ROOT_VERSION} build options.")
  endif()
endforeach()

#---Deprecated options------------------------------------------------------------------------
foreach(opt )
  if(${opt})
    message(DEPRECATION ">>> Option '${opt}' is deprecated and will be removed in the next release of ROOT. Please contact root-dev@cern.ch should you still need it.")
  endif()
endforeach()

foreach(opt minuit2_mpi)
  if(${opt})
      message(WARNING "The option '${opt}' can only be used to minimise thread-safe functions in Minuit2. It cannot be used for Histogram/Graph fitting and for RooFit. If you want to use Minuit2 with MPI support, it is better to build Minuit2 as a standalone library.")
  endif()
endforeach()

#---Avoid creating dependencies to 'non-standard' header files -------------------------------
include_regular_expression("^[^.]+$|[.]h$|[.]icc$|[.]hxx$|[.]hpp$")

#---Add Installation Variables------------------------------------------------------------------
include(RootInstallDirs)

#---RPATH options-------------------------------------------------------------------------------

# add to RPATH any directories outside the project that are in the linker search path
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

if(rpath)
  file(RELATIVE_PATH BINDIR_TO_LIBDIR "${CMAKE_INSTALL_FULL_BINDIR}" "${CMAKE_INSTALL_FULL_LIBDIR}")

  if(APPLE)
    set(CMAKE_MACOSX_RPATH TRUE)
    set(CMAKE_INSTALL_NAME_DIR "@rpath")

    set(_rpath_values "@loader_path" "@loader_path/${BINDIR_TO_LIBDIR}")
  else()
    set(_rpath_values "$ORIGIN" "$ORIGIN/${BINDIR_TO_LIBDIR}")
  endif()

  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH-CACHED};${_rpath_values}" CACHE STRING "Install RPATH" FORCE)

  unset(BINDIR_TO_LIBDIR)
endif()

#---deal with the DCMAKE_IGNORE_PATH------------------------------------------------------------
if(macos_native)
  if(APPLE)
    set(CMAKE_IGNORE_PATH)
    foreach(_prefix /sw /opt/local /usr/local /opt/homebrew) # Fink installs in /sw, and MacPort in /opt/local and Brew in /usr/local (x86-64) and /opt/homebrew (arm64)
      list(APPEND CMAKE_IGNORE_PATH ${_prefix}/bin ${_prefix}/include ${_prefix}/lib)
    endforeach()
    if(CMAKE_VERSION VERSION_GREATER 3.15)
      # Bug was reported on newer version of CMake on Mac OS X:
      # https://gitlab.kitware.com/cmake/cmake/-/issues/19662
      # https://github.com/microsoft/vcpkg/pull/7967
      set(builtin_glew_defvalue ON)
    endif()
  else()
    message(STATUS "Option 'macos_native' is only for MacOS systems. Ignoring it.")
  endif()
endif()


#---distributed RDataFrame pyspark tests require both dataframe and pyroot----------------------------------
if(test_distrdf_pyspark OR test_distrdf_dask AND (NOT dataframe OR NOT pyroot))
    message(FATAL_ERROR "Running the tests for distributed RDataFrame requires both RDataFrame and PyROOT to be enabled (build with -Ddataframe=ON and -Dpyroot=ON)")
endif()

#---TPython requires PyROOT
if(tpython AND NOT pyroot)
    message(FATAL_ERROR "-Dtpython=ON requires -Dpyroot=ON)")
endif()
