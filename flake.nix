{
  description = "PTP daemon (PTPd) is an implementation the Precision Time Protocol (PTP) version
2 as defined by 'IEEE Std 1588-2008'. PTP provides precise time coordination of
Ethernet LAN connected computers. It was designed primarily for instrumentation
and control systems.";

  # Nixpkgs / NixOS version to use.
  inputs.nixpkgs.url = "nixpkgs/nixos-21.05";

  outputs = { self, nixpkgs }:
    let
      # Generate a user-friendly version number.
      version = builtins.substring 0 8 self.lastModifiedDate;

      # System types to support.
      supportedSystems = [ "x86_64-linux" "x86_64-darwin" "aarch64-linux" "aarch64-darwin" ];

      # Helper function to generate an attrset '{ x86_64-linux = f "x86_64-linux"; ... }'.
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;

      # Nixpkgs instantiated for supported system types.
      nixpkgsFor = forAllSystems (system: import nixpkgs { inherit system; overlays = [ self.overlay ]; });

    in

    {

      # A Nixpkgs overlay.
      overlay = final: prev: {

        ptpd = with final; stdenv.mkDerivation rec {
          name = "ptpd-${version}";

          src = ./.;

          nativeBuildInputs = [ autoreconfHook ];
          buildInputs = [ libpcap ];
        };

      };

      # Provide some binary packages for selected system types.
      packages = forAllSystems (system:
        {
          inherit (nixpkgsFor.${system}) ptpd;
        });

      # The default package for 'nix build'. This makes sense if the
      # flake provides only one package or there is a clear "main"
      # package.
      defaultPackage = forAllSystems (system: self.packages.${system}.ptpd);

      # A NixOS module, if applicable (e.g. if the package provides a system service).
      nixosModules.ptpd =
        { pkgs, ... }:
        {
          nixpkgs.overlays = [ self.overlay ];
          environment.systemPackages = [ pkgs.ptpd ];
        };

    };
}
