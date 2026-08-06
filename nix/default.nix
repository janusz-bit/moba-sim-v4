{ inputs, ... }:
{
  systems = [
    "x86_64-linux"
    "aarch64-linux"
  ];

  imports = [ inputs.git-hooks-nix.flakeModule ];

  perSystem =
    {
      config,
      pkgs,
      self',
      ...
    }:
    let
      pkg = self'.packages.default;
    in
    {
      formatter = pkgs.nixfmt-tree;

      pre-commit.settings.hooks = {
        nixfmt.enable = true;
        clang-format.enable = true;
      };

      packages.default = pkgs.clangStdenv.mkDerivation {
        pname = "moba-sim";
        version = "0.1.0";
        src = ./..;

        # The test binary lands in a separate output, so `nix run .#tests`
        # reuses this build without polluting the release output.
        outputs = [
          "out"
          "tests"
        ];

        nativeBuildInputs = [
          pkgs.cmake
          pkgs.ninja
        ];

        buildInputs = [
          pkgs.catch2_3
          pkgs.sdl3
        ];

        doCheck = true;

        # The setup hook has already cd'd into the build directory, hence ".".
        postInstall = ''
          cmake --install . --prefix "$tests" --component tests
        '';
      };

      # Generated API reference. Separate from packages.default so building the
      # simulator never pulls in doxygen + graphviz, and so warnings about
      # undocumented public API fail this derivation rather than the build.
      packages.docs = pkgs.stdenv.mkDerivation {
        pname = "moba-sim-docs";
        version = "0.1.0";
        src = ./..;

        nativeBuildInputs = [
          pkgs.cmake
          pkgs.ninja
          pkgs.doxygen
          pkgs.graphviz
        ];

        # Nothing is compiled or linked here -- doxygen only parses headers --
        # but the CMake configure step still runs find_package for the whole
        # project, so SDL3 and Catch2 have to be present.
        buildInputs = [
          pkgs.catch2_3
          pkgs.sdl3
        ];

        cmakeFlags = [
          "-DMOBA_SIM_BUILD_DOCS=ON"
          "-DMOBA_SIM_DOCS_WARNINGS_AS_ERRORS=ON"
          # Tests are configured but never built; skip discovering them.
          "-DBUILD_TESTING=OFF"
        ];

        # graphviz renders the diagrams and wants a writable font cache; the
        # sandbox has no HOME, so point both at the build directory. Without
        # this the log fills with Fontconfig errors that hide real warnings.
        preBuild = ''
          export HOME="$TMPDIR"
          export XDG_CACHE_HOME="$TMPDIR/cache"
          mkdir -p "$XDG_CACHE_HOME/fontconfig"
        '';

        buildPhase = ''
          runHook preBuild
          cmake --build . --target docs
          runHook postBuild
        '';

        installPhase = ''
          runHook preInstall
          mkdir -p "$out/share/doc/moba-sim"
          cp -r docs/html "$out/share/doc/moba-sim/html"
          runHook postInstall
        '';

        doCheck = false;
      };

      checks.default = pkg;
      checks.docs = self'.packages.docs;

      apps =
        let
          app = program: {
            type = "app";
            inherit program;
          };

          # `nix run` only accepts a bare executable path, so the headless test
          # suite (game_loop.test.cpp opens a window) needs this wrapper.
          tests-wrapper = pkgs.writeShellApplication {
            name = "moba-sim-tests";
            text = ''
              export SDL_VIDEO_DRIVER="''${SDL_VIDEO_DRIVER:-dummy}"
              exec ${pkg.tests}/bin/moba_sim_tests "$@"
            '';
          };

          # Prints the path to the generated docs, or opens them when a browser
          # is available. Printing unconditionally keeps it useful over SSH and
          # in CI, where xdg-open would either fail or hang.
          docs-wrapper = pkgs.writeShellApplication {
            name = "moba-sim-docs";
            runtimeInputs = [ pkgs.xdg-utils ];
            text = ''
              index="${self'.packages.docs}/share/doc/moba-sim/html/index.html"
              echo "$index"
              if [ -n "''${DISPLAY:-}''${WAYLAND_DISPLAY:-}" ]; then
                xdg-open "$index" >/dev/null 2>&1 || true
              fi
            '';
          };
        in
        {
          moba-sim = app "${pkg}/bin/moba-sim";
          moba-sim-view = app "${pkg}/bin/moba-sim-view";
          tests = app "${tests-wrapper}/bin/moba-sim-tests";
          docs = app "${docs-wrapper}/bin/moba-sim-docs";
          default = self'.apps.moba-sim;
        };

      devShells.default = (pkgs.mkShell.override { stdenv = pkgs.clangStdenv; }) {
        inputsFrom = [ pkg ];
        shellHook = config.pre-commit.shellHook;
        packages = config.pre-commit.settings.enabledPackages ++ [
          pkgs.clang-tools
          # `cmake --build build --target docs`
          pkgs.doxygen
          pkgs.graphviz
        ];
      };
    };
}
