{
  description = "Music Visualizer";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
  };

  outputs = { self, nixpkgs }:
  let
    system = "x86_64-linux";
    pkgs = nixpkgs.legacyPackages.${system};
    ldLibraryPath = with pkgs; lib.makeLibraryPath [ xorg.libX11 libglvnd alsa-lib ];
  in
  {
    packages.${system}.default =
      with pkgs; stdenv.mkDerivation {
        name = "musializer";

        src = ./.;

        nativeBuildInputs = [ makeWrapper ];
        buildInputs = with xorg; [
          libX11
          libXcursor
          libXrandr
          libXinerama
          libXi
        ];

        buildPhase = ''
          cc -o nob nob.c
          ./nob
        '';

        installPhase = ''
          mkdir -p $out/bin
          mv build/musializer $out/bin/musializer
          wrapProgram $out/bin/musializer \
          --prefix LD_LIBRARY_PATH : ${ldLibraryPath} \
          --prefix PATH : ${lib.makeBinPath [ ffmpeg ]}
        '';
      };

    devShells.${system}.default =
      with pkgs; mkShell {
        name = "musializer-shell";
        inputsFrom = [ self.packages.${system}.default ];
        shellHook = "export LD_LIBRARY_PATH=${ldLibraryPath}:$LD_LIBRARY_PATH";
      };
  };
}
