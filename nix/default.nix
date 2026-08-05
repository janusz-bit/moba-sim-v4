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

      checks.default = pkg;

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
        in
        {
          moba-sim = app "${pkg}/bin/moba-sim";
          moba-sim-view = app "${pkg}/bin/moba-sim-view";
          tests = app "${tests-wrapper}/bin/moba-sim-tests";
          default = self'.apps.moba-sim;
        };

      devShells.default = (pkgs.mkShell.override { stdenv = pkgs.clangStdenv; }) {
        inputsFrom = [ pkg ];
        shellHook = config.pre-commit.shellHook;
        packages = config.pre-commit.settings.enabledPackages ++ [ pkgs.clang-tools ];
      };
    };
}
