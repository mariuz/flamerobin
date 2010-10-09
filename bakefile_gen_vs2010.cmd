@rem - Bakefile_gen will generate flamerobin.sln and *.vcproj files.
@rem - This script also creates a copy of flamerobin.sln under
@rem - flamerobin_vs2010.sln. Open flamerobin_vs2010.sln in
@rem - Visual Studio 2010 and it will convert and create *.vcxproj.

@bakefile_gen
@copy flamerobin.sln flamerobin_vs2010.sln
