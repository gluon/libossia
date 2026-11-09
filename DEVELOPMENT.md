# Development notes

## The build SHA of a package

When it loads, the Max package prints `build SHA : <commit>` in the console (`ossia::get_commit_sha()`, `src/ossia-max/src/ossia-max.cpp`). That commit is read when CMake configures: `src/ossia_ci.cmake` asks git for `HEAD` and writes it into `<build folder>/src/git_info.cpp`. `cmake --build` reads no commit.

In a build folder that is kept, the stamp is therefore the `HEAD` of the last configure:

- A commit made after that configure does not change it, even when the build runs after the commit.
- The stamp says nothing about the working tree: changes not yet committed are compiled, and do not show.
- The version of the package is read from the sources, not from git. A package can say `2.1.0+prism.11` with the commit before the one that set that version.

To stamp a local build with the current `HEAD`, commit first, then configure again and build:

```bash
cmake -S . -B build-ossia-max
cmake --build build-ossia-max
```

Any commit after that leaves the stamp one commit behind, a commit of documentation included.

A Release is not concerned: it is built from its tag in a clean clone, configured once at that tag. The build of `v2.1.0+prism.10` holds `87e7fad2f`, the commit of the tag.
