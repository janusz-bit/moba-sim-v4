{ inputs, ... }:
{
  systems = [
    "x86_64-linux"
    "aarch64-linux"
  ];

  imports = [
    inputs.git-hooks-nix.flakeModule
  ];

  perSystem =
    {
      config,
      lib,
      pkgs,
      self',
      ...
    }:
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
        # reuses this build instead of compiling everything again, while the
        # default output stays free of test binaries.
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

        # Place the test binary in the `tests` output, reusing this build. The
        # setup hook has already cd'd into the build directory, hence ".".
        postInstall = ''
          cmake --install . --prefix "$tests" --component tests
        '';
      };

      checks.default = self'.packages.default;

      apps =
        let
          # Runs `program` from `package`, forwarding arguments. `nix run` only
          # accepts a bare executable path, so anything needing a wrapper (an
          # env var, a different output) goes through writeShellApplication.
          runner =
            {
              name,
              package ? self'.packages.default,
              program ? name,
              env ? { },
            }:
            let
              exports = lib.concatStringsSep "\n" (
                lib.mapAttrsToList (key: value: ''export ${key}="''${${key}:-${value}}"'') env
              );
              wrapper = pkgs.writeShellApplication {
                inherit name;
                text = ''
                  ${exports}
                  exec ${lib.getExe' package program} "$@"
                '';
              };
            in
            {
              type = "app";
              program = lib.getExe wrapper;
            };
        in
        {
          # Console demo: stat pipeline, effects over simulated time, events.
          moba-sim = runner { name = "moba-sim"; };

          # Interactive SDL3 demo. SDL_VIDEO_DRIVER is left to SDL's own
          # detection unless the caller sets it, so this works on Wayland and
          # X11 alike -- and `SDL_VIDEO_DRIVER=dummy nix run .#moba-sim-view`
          # still works headlessly.
          moba-sim-view = runner { name = "moba-sim-view"; };

          # Catch2 suite. Arguments pass straight through, so tag filters work:
          #   nix run .#tests -- "[effects]"
          #   nix run .#tests -- --list-tests
          # The dummy video driver is forced because the suite must stay
          # headless: game_loop.test.cpp opens a window.
          tests = runner {
            name = "moba-sim-tests";
            package = self'.packages.default.tests;
            program = "moba_sim_tests";
            env.SDL_VIDEO_DRIVER = "dummy";
          };

          default = self'.apps.moba-sim;
        };

      devShells.default = pkgs.mkShell.override { stdenv = pkgs.clangStdenv; } {
        inputsFrom = [ self'.packages.default ];

        shellHook = ''
          ${config.pre-commit.shellHook}
        '';

        packages = config.pre-commit.settings.enabledPackages ++ [
          # clang-format / clang-tidy
          pkgs.clang-tools
        ];
      };
    };
}
