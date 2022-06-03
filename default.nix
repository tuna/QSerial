with import <nixpkgs> {};

stdenv.mkDerivation {
  name = "QSerial";
  version = "1.1";

  src = ./.;

  nativeBuildInputs = [
    cmake
    pkgconfig
    libsForQt5.wrapQtAppsHook
  ];

  buildInputs = [
    libsForQt5.qt5.qtbase
    libsForQt5.qt5.qtserialport
    libsForQt5.qt5.qtwebengine
    libusb1
  ];
}

