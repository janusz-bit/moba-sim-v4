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

        nativeBuildInputs = [
          pkgs.cmake
          pkgs.ninja
        ];

        buildInputs = [
          pkgs.catch2_3
          pkgs.sdl3
        ];

        doCheck = true;
      };

      checks.default = self'.packages.default;

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
