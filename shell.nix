{ nixpkgs ? import <nixpkgs> {},
  rubypkg ? "ruby"
}:

with nixpkgs;

stdenv.mkDerivation {
  name = "ruby-${ruby.version}-env";

  buildInputs = [
    nixpkgs.${rubypkg}

    pkgconfig
    libxml2                     # nokogiri
    libxslt                     # nokogiri
    zlib                        # nokogiri, others
  ];

  shellHook = ''
    export BUNDLE_BULID__NOKOGIRI="--use-system-libraries"
  '';
}
