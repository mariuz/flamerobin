# Using Firebird Embedded in FlameRobin

FlameRobin can open a local database file directly, with no Firebird server process
involved, by loading the Firebird **engine** into its own address space. This is what
Firebird calls *embedded* mode.

Embedded is not a separate library. It is the ordinary client library (`fbclient.dll`
on Windows, `libfbclient.so` on Linux and macOS) plus an **engine plugin** next to it,
and it is the presence of that plugin that decides whether embedded works at all.

---

## 1. How Firebird decides between embedded and the network

When FlameRobin hands the client library a connection string, the client walks the
`Providers` list from `firebird.conf`, which by default is:

```text
Providers = Remote, Engine13, Loopback
```

| provider | takes | what it does |
| --- | --- | --- |
| `Remote` | `host:/path/db.fdb` | connects to a Firebird server over the network |
| `Engine13` | `/path/db.fdb` | **embedded** - opens the file in this process |
| `Loopback` | `/path/db.fdb` | connects to `localhost` over the network |

A path with no host is offered to `Remote`, which declines it, then to the engine. If
the engine plugin is **missing**, the path falls through to `Loopback`, which quietly
turns your local file into a network connection to `localhost`. When nothing is
listening there, the connection fails with:

```text
Unable to complete network request to host "localhost".
-Failed to establish a connection.
```

That error means *the engine plugin is not there*, even though it says nothing about
embedded mode. Since 26.9.5 FlameRobin recognises this case and appends an explanation
to the message instead of leaving you with the bare network error.

> [!NOTE]
> The engine plugin is version-locked to the database file. `Engine13` opens ODS 13.0
> and 13.1, the on-disk structures of Firebird 4 and Firebird 5. An ODS 12 database
> (Firebird 3) needs `Engine12` from a Firebird 3 installation, and older files need
> older engines still. See [Which engine for which database](#5-which-engine-for-which-database).

---

## 2. Registering an embedded database

The rule is simple: **a server with no host name is an embedded connection.** FlameRobin
builds the connection string from the server's host and port, so leaving both empty
passes the bare file path to the client library.

### Step 1: Register the server

1. In the metadata tree, right-click the root node and choose **Register new server**
   (or right-click an existing server and choose **Server registration info**).
2. Fill in:
   - **Name (Display Name)**: anything descriptive, for example `Embedded`.
   - **Host (Server)**: leave **completely empty**.
   - **Port**: leave **completely empty**.
3. Click **Save**.

### Step 2: Register the database

1. Right-click the new server and choose **Register existing database**.
2. Fill in:
   - **Display name**: anything descriptive.
   - **Database path**: the **absolute local path**, for example `C:\Data\mydb.fdb` or
     `/home/you/data/mydb.fdb`.
     > [!IMPORTANT]
     > Never prefix the path with `localhost:` or `127.0.0.1:`. A prefix makes the
     > client take the network route no matter what else is configured.
   - **Username / Password**: `SYSDBA` / `masterkey` is the usual pair. In embedded
     mode the engine authorises on operating system file permissions and does not
     verify the password.
   - **Client library**: usually empty - see the next section.
3. Click **Save**, then double-click the database to connect.

The stored registration looks like this in `fr_databases.conf`:

```xml
<server>
  <name>Embedded</name>
  <database>
    <id>-21</id>
    <name>My database</name>
    <path>/home/you/data/mydb.fdb</path>
    <charset>UTF8</charset>
    <username>SYSDBA</username>
    <password>masterkey</password>
  </database>
</server>
```

Note the absent `<host>` and `<port>` elements. If you see `<host>localhost</host>`
there, the connection is a network connection, not an embedded one.

---

## 3. Choosing the client library per database

Each registered database has a **Client library** field (`<fbclient>` in
`fr_databases.conf`). Leave it empty to use the client FlameRobin ships with; set it
to an absolute path to load a different one, which is how you point an embedded
database at a Firebird installation whose engine matches the file:

```text
/opt/firebird/lib/libfbclient.so
/usr/lib/x86_64-linux-gnu/libfbclient.so.2
C:\Program Files\Firebird\Firebird_5_0\fbclient.dll
```

The library is loaded together with everything that sits next to it - its `plugins`
directory, its `firebird.conf`, its `intl` module - so a client taken from a complete
installation brings its own engine along.

> [!NOTE]
> Before 26.9.5 this setting was effectively global: the first database to connect
> decided the client library for the whole application, and every later database
> silently reused it. Each database now gets the library it is configured with, so
> an embedded database and a remote one can run side by side on different clients.

---

## 4. What each FlameRobin package ships

| package | client | engine included | embedded out of the box |
| --- | --- | --- | --- |
| Linux `.deb` (26.9.5 and later) | bundled, in `/opt/flamerobin/lib` | yes, `/opt/flamerobin/plugins/firebird/libEngine13.so` | yes, for ODS 13.0 / 13.1 |
| Linux `.deb` (26.9.4 and earlier) | bundled, client only | **no** | no - falls back to `localhost` |
| Windows portable | bundled | yes | yes |
| macOS | the client of the official Firebird `.pkg` | from that installation | yes, if the `.pkg` is installed |
| distribution packages | the distribution's `libfbclient` | only with the server package installed | install `firebird3.0-server`, `firebird4.0-server` or similar |

The Linux `.deb` lays the Firebird tree out the way the client expects to find it,
because `libfbclient` resolves everything relative to its own location:

```text
/opt/flamerobin/lib/libfbclient.so.2        the client
/opt/flamerobin/plugins/firebird/           Engine13, Srp, ChaCha, fbtrace, ...
/opt/flamerobin/intl/                       character sets and collations
/opt/flamerobin/share/firebird/firebird.msg error message texts
/opt/flamerobin/share/firebird/tzdata/      time zone database
/opt/flamerobin/firebird.conf               documented, everything commented out
```

---

## 5. Which engine for which database

| database created by | ODS | engine plugin |
| --- | --- | --- |
| Firebird 5.0 | 13.1 | `Engine13` |
| Firebird 4.0 | 13.0 | `Engine13` |
| Firebird 3.0 | 12 | `Engine12` |
| Firebird 2.5 | 11.2 | no plugin architecture - use the 2.5 client |

To open an ODS 12 database with the bundled client, copy `libEngine12.so` (or
`engine12.dll`) out of a Firebird 3 installation into the plugins directory and list
it in `firebird.conf`:

```text
Providers = Remote, Engine12, Engine13, Loopback
```

Firebird tries each engine in turn and the one that recognises the on-disk structure
wins. Alternatively, back up and restore the database with the newer version, or point
the database's **Client library** at the Firebird 3 installation.

---

## 6. Command line

### Firebird tools

Pass the absolute path with no network prefix:

```bash
isql /home/you/data/mydb.fdb -user SYSDBA -password masterkey
```

### FlameRobin

A database file passed on the FlameRobin command line is registered under the virtual
server **Unregistered local databases**, whose host name is `localhost`
([root.cpp](../src/metadata/root.cpp)). The connection therefore goes over the network,
not through the engine. Register the database through the GUI, as in section 2, to get
an embedded connection.

---

## 7. Troubleshooting

**`Unable to complete network request to host "localhost"`**
The engine plugin is missing, so the connection fell through to `Loopback`. Check that
the plugins directory next to the client library contains an `Engine*` module, or point
the database's **Client library** at an installation that has one. Help -> About reports
the plugin directory and the plugin modules installed in it, which is the quickest way
to see whether an engine is present - look for an `Engine*` entry.

**`unsupported on-disk structure for file ...`**
The engine is there but it is the wrong version for this file. See
[Which engine for which database](#5-which-engine-for-which-database).

**`operating system directive access failed` on `/tmp/firebird`**
Embedded needs a writable lock directory, `/tmp/firebird` by default. A Firebird server
package may have created it for its own user, leaving your account without access.
Either add yourself to that group, or point `FIREBIRD_LOCK` at a directory of your own
before starting FlameRobin.

**The database is already open elsewhere**
By default the engine takes the database file for itself, and a database already opened
by a Firebird server or another embedded process will refuse to open. Firebird 3.0 and
later can share a local file between processes through shared lock files - set this in
the `firebird.conf` next to the client library:

```text
ServerMode = SuperClassic
```

> [!WARNING]
> Never open a file in embedded mode while a Firebird server has it open unless both
> use the same lock directory. Two engines writing the same file with separate lock
> tables will corrupt it.

**Character set or collation errors**
The `intl` module is missing. Only `NONE`, `OCTETS`, `ASCII`, `UNICODE_FSS` and `UTF8`
are built into the engine; everything else lives in `intl/fbintl`, which has to sit in
the Firebird tree next to the client library.
