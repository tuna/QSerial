{ pkgs ? import <nixpkgs> {}
}:

pkgs.mkShell {
  buildInputs = [
    pkgs.libsForQt5.qt5.qtbase
    pkgs.libsForQt5.qt5.qtserialport
    pkgs.libsForQt5.qt5.qtwebengine
    pkgs.libusb1
    pkgs.cmake
    pkgs.pkgconfig
  ];
}
