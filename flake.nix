{
  description = "A development flake for crynth";

  inputs.nixpkgs.url = "https://flakehub.com/f/NixOS/nixpkgs/0.1";

  outputs = inputs: let
    supportedSystems = [
      "x86_64-linux" # 64-bit Intel/AMD Linux
      "aarch64-linux" # 64-bit ARM Linux
    ];

    forEachSupportedSystem = f:
      inputs.nixpkgs.lib.genAttrs supportedSystems (
        system:
          f {
            pkgs = import inputs.nixpkgs {inherit system;};
          }
      );
  in {
    devShells = forEachSupportedSystem (
      {pkgs}: {
        default = pkgs.mkShell {
          packages = with pkgs; [
            clang
            stdenv.cc
            lldb

            man-pages
            man-pages-posix

            alsa-lib

            raylib
          ];
        };
      }
    );

    packages = forEachSupportedSystem ({pkgs}: {
      default = pkgs.stdenv.mkDerivation rec {
        name = "crynth";
        src = ./.;
        buildInputs = with pkgs; [
          raylib
          alsa-lib
        ];

        buildPhase = ''
          cc -o nob nob.c
          ./nob
        '';

        installPhase = ''
          mkdir -p $out/bin
          cp ./build/crynth $out/bin
        '';
      };
    });
  };
}
