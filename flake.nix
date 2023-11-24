{
  inputs = {
    nixpkgs-stable.url = "github:nixos/nixpkgs/nixos-23.05";
    nixpkgs-trunk.url = "github:nixos/nixpkgs";
    flake-compat = {
      url = "github:edolstra/flake-compat";
      flake = false;
    };
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs-stable, nixpkgs-trunk, flake-compat, flake-utils }:
    let
      ton = { host, pkgs ? host, stdenv ? pkgs.stdenv, staticGlibc ? false
        , staticMusl ? false, staticExternalDeps ? staticGlibc }:
        with host.lib;
        stdenv.mkDerivation {
          pname = "ton";
          version = "dev";

          src = ./.;

          nativeBuildInputs = with host;
            [ cmake ninja pkg-config git ] ++
            optionals stdenv.isLinux [ dpkg rpm createrepo_c pacman ];
          buildInputs = with pkgs;
          # at some point nixpkgs' pkgsStatic will build with static glibc
          # then we can skip these manual overrides
          # and switch between pkgsStatic and pkgsStatic.pkgsMusl for static glibc and musl builds
            if !staticExternalDeps then [
              openssl
              zlib
              libmicrohttpd
              libsodium
              secp256k1
            ] else
              [
                (openssl.override { static = true; }).dev
                (zlib.override { shared = false; }).dev
            ]
            ++ optionals (!stdenv.isDarwin) [ pkgsStatic.libmicrohttpd.dev pkgsStatic.libsodium.dev secp256k1 ]
            ++ optionals stdenv.isDarwin [ (libiconv.override { enableStatic = true; enableShared = false; }) ]
            ++ optionals stdenv.isDarwin (forEach [ libmicrohttpd.dev libsodium.dev secp256k1 gmp.dev nettle.dev (gnutls.override { withP11-kit = false; }).dev libtasn1.dev libidn2.dev libunistring.dev gettext ] (x: x.overrideAttrs(oldAttrs: rec { configureFlags = (oldAttrs.configureFlags or []) ++ [ "--enable-static" "--disable-shared" ]; dontDisableStatic = true; })))
            ++ optionals staticGlibc [ glibc.static ];

          dontAddStaticConfigureFlags = stdenv.isDarwin;

          cmakeFlags = [ "-DTON_USE_ABSEIL=OFF" "-DNIX=ON" ] ++ optionals staticMusl [
            "-DCMAKE_CROSSCOMPILING=OFF" # pkgsStatic sets cross
          ] ++ optionals (staticGlibc || staticMusl) [
            "-DCMAKE_LINK_SEARCH_START_STATIC=ON"
            "-DCMAKE_LINK_SEARCH_END_STATIC=ON"
          ] ++ optionals (stdenv.isDarwin) [
            "-DCMAKE_CXX_FLAGS=-stdlib=libc++" "-DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=11.7"
          ];

          LDFLAGS = optional staticExternalDeps (concatStringsSep " " [
            (optionalString stdenv.cc.isGNU "-static-libgcc")
            (optionalString stdenv.isDarwin "-framework CoreFoundation")
            "-static-libstdc++"
          ]);

          GIT_REVISION = if self ? rev then self.rev else "dirty";
          GIT_REVISION_DATE = (builtins.concatStringsSep "-" (builtins.match "(.{4})(.{2})(.{2}).*" self.lastModifiedDate)) + " " + (builtins.concatStringsSep ":" (builtins.match "^........(.{2})(.{2})(.{2}).*" self.lastModifiedDate));

          postInstall = ''
            moveToOutput bin "$bin"
          '';

          preFixup = optionalString stdenv.isDarwin ''
            for fn in "$bin"/bin/* "$out"/lib/*.dylib; do
              echo Fixing libc++ in "$fn"
              install_name_tool -change "$(otool -L "$fn" | grep libc++.1 | cut -d' ' -f1 | xargs)" libc++.1.dylib "$fn"
              install_name_tool -change "$(otool -L "$fn" | grep libc++abi.1 | cut -d' ' -f1 | xargs)" libc++abi.dylib "$fn"
            done
          '';

          outputs = [ "bin" "out" ];
        };
      hostPkgs = system:
        import nixpkgs-stable {
          inherit system;
          overlays = [
            (self: super: {
              zchunk = nixpkgs-trunk.legacyPackages.${system}.zchunk;
            })
          ];
        };
    in with flake-utils.lib;
    (nixpkgs-stable.lib.recursiveUpdate
      (eachSystem (with system; [ x86_64-linux aarch64-linux ]) (system:
        let host = hostPkgs system;
        in rec {
          packages = rec {
            ton-normal = ton { inherit host; };
            ton-static = ton {
              inherit host;
              stdenv = host.makeStatic host.stdenv;
              staticGlibc = true;
            };
            ton-musl =
              let pkgs = nixpkgs-stable.legacyPackages.${system}.pkgsStatic;
              in ton {
                inherit host;
                inherit pkgs;
                stdenv = pkgs.gcc12Stdenv;
                staticMusl = true;
              };
            ton-oldglibc =
              let pkgs = nixpkgs-stable.legacyPackages.${system}.pkgsStatic;
              in ton {
              inherit host;
              inherit pkgs;
              stdenv = pkgs.gcc12Stdenv;
              staticExternalDeps = true;
            };
            ton-oldglibc_staticbinaries = host.symlinkJoin {
              name = "ton";
              paths = [ ton-musl.bin ton-oldglibc.out ];
            };
          };
          devShells.default =
            host.mkShell { inputsFrom = [ packages.ton-normal ]; };
        })) (eachSystem (with system; [ x86_64-darwin aarch64-darwin ]) (system:
          let host = hostPkgs system;
          in rec {
            packages = rec {
              ton-normal = ton { inherit host; };
              ton-static = ton {
                inherit host;
                stdenv = host.makeStatic host.stdenv;
                staticExternalDeps = true;
              };
              ton-staticbin-dylib = host.symlinkJoin {
                name = "ton";
                paths = [ ton-static.bin ton-static.out ];
              };
            };
            devShells.default =
              host.mkShell { inputsFrom = [ packages.ton-normal ]; };
          })));
}