Please take care when mixing installing / copying of 32 and 64 bit versions of FlameRobin.

The installations use different IDs and will both appear in the "Add / Remove Programs" applet, it's therefore best to install them into different directories.  When copying files from the ZIP packages please make sure that the 64 bit version is always copied together with the manifest file, while the manifest file has to be removed when the 32 bit version is used.  Keeping the manifest file of the 64 bit version in the same directory as the 32 bit executable makes running it impossible.

Firebird client library (fbclient.dll) is required.

FlameRobin requires the Firebird client library (fbclient.dll) to connect to Firebird databases. You can obtain it by:
  1. Installing Firebird from https://firebirdsql.org/en/firebird-5-0/ (recommended), or
  2. Copying fbclient.dll from an existing Firebird installation into the FlameRobin installation directory.
