{ stdenv
, lib
, ffmpeg
, libGL
, callPackage
, glfw
, libXcursor
, libXrandr
, libXinerama
, alsa-lib
, mesa
, libXi
, darwin
, makeBinaryWrapper
, ldLibraryPath
}:
let
  inherit (darwin.apple_sdk.frameworks) Carbon Cocoa OpenGL;

  target =
    if stdenv.hostPlatform.isLinux then "LINUX"
    else if stdenv.hostPlatform.isDarwin then "MACOS"
    else if stdenv.hostPlatform.isOpenBSD then "OPENBSD"
    else lib.throwIf true "Unsupported platform: ${stdenv.hostPlatform.system}";
in
stdenv.mkDerivation {
  pname = "musializer";
  version = "0.alpha-3";
  src = lib.cleanSource ./..;

  nativeBuildInputs = [
    makeBinaryWrapper
  ];

  buildInputs =
    [
      glfw
    ]
    ++ lib.optionals stdenv.hostPlatform.isLinux [
      mesa
      libXi
      libXcursor
      libXrandr
      libXinerama
      libGL
      alsa-lib
    ]
    ++ lib.optionals stdenv.hostPlatform.isDarwin [
      Carbon
      Cocoa
      OpenGL
    ];

  buildPhase = ''
    runHook preBuild

    cc -o nob nob.c
    mkdir -p build
    echo '#define MUSIALIZER_TARGET_${target}' > build/config.h
    ./nob

    runHook postBuild
  '';

  installPhase = ''
    runHook preInstall

    install -Dm755 build/musializer -t $out/bin
    wrapProgram $out/bin/musializer \
      --prefix PATH : ${lib.makeBinPath [ ffmpeg ]} \
      --prefix LD_LIBRARY_PATH : ${callPackage ldLibraryPath {}}

    runHook postInstall
  '';
}
