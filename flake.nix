{
  description = "Music Visualizer";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs, ... }@inputs:
    let
      inherit (nixpkgs) lib;
      systems = [ "x86_64-linux" "aarch64-linux" ];
      eachSystem = f: lib.genAttrs systems f;
      pkgsFor = eachSystem (system:
        import nixpkgs {
          inherit system;
          overlays = [
            self.overlays.default
          ];
        });
    in
    {
      ldLibraryPath =
        { libX11
        , libGL
        , alsa-lib
        }:
        lib.makeLibraryPath [ libX11 libGL alsa-lib ];

      overlays = import ./nix/overlays.nix { inherit self lib inputs; };

      packages = eachSystem (system:
        let
          pkgs = pkgsFor.${system};
        in
        {
          default = self.packages.${system}.musializer;
          inherit (pkgs) musializer;
        });

      devShells = eachSystem (system:
        let
          pkgs = pkgsFor.${system};
        in
        {
          default = pkgs.mkShell {
            name = lib.getName self.packages.${system}.default + "-shell";
            LD_LIBRARY_PATH = "${pkgs.callPackage self.ldLibraryPath {}}:$LD_LIBRARY_PATH";
            inputsFrom = [
              self.packages.${system}.default
            ];
          };
        }
      );
    };
}
