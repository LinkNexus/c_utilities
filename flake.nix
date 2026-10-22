{
  description = "c_utils: C23 single-header utilities";

  # Pinned, not tracking a branch: the point of a project shell is that
  # its toolchain only moves when someone moves it. Same revision as the
  # machine's system flake, so everything here is already in the store.
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/8edc0c72e3a38faf5434e40d1f19431125fe4b30";

  outputs = {
    self,
    nixpkgs,
  }: let
    forEachSystem =
      nixpkgs.lib.genAttrs
      ["aarch64-darwin" "x86_64-darwin" "x86_64-linux" "aarch64-linux"];
  in {
    devShells = forEachSystem (system: let
      pkgs = nixpkgs.legacyPackages.${system};
    in {
      default = pkgs.mkShell {
        nativeBuildInputs = [
          # `cc` for the Makefile's CC, plus C23 support -- mkShell
          # supplies the host stdenv's compiler.
          pkgs.gnumake
          pkgs.cmake # for consumers of the INTERFACE library target

          # compile_commands.json, which the LSP needs to resolve the
          # -I. and -std=c23 this project builds with. Lives here rather
          # than in the global package set because it is a build tool for
          # this project, not something used outside one.
          pkgs.bear
        ];
      };
    });
  };
}
