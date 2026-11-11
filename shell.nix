{
  pkgs ? import <nixpkgs> { },
}:
pkgs.mkShell {
  buildInputs = with pkgs; [
    boost.dev
    (python3.withPackages (
      ps: with ps; [
        matplotlib
      ]
    ))
  ];

  hardeningDisable = [ "fortify" ];
}
