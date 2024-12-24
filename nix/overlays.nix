{ self
, lib
, inputs
}: {
  default = final: prev: {
    musializer = final.callPackage ./default.nix { inherit (self) ldLibraryPath; };
  };
}
