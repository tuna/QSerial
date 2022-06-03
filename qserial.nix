{ stdenv, qtbase, qtserialport, qtwebengine, wrapQtAppsHook, cmake, pkgconfig, libusb1 }:

stdenv.mkDerivation {
  name = "QSerial";
  version = "1.1";

  src = ./.;

  nativeBuildInputs = [
    cmake
    pkgconfig
    wrapQtAppsHook
  ];

  buildInputs = [
    qtbase
    qtserialport
    qtwebengine
    libusb1
  ];
 
  # https://github.com/NixOS/nixpkgs/issues/66755#issuecomment-657305962
  # Fix "Could not initialize GLX" error
  postInstall = ''
    wrapProgram "$out/bin/QSerial" --set QT_XCB_GL_INTEGRATION none
  '';
}

