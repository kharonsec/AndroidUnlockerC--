pub fn generate_compose() -> &'static str {
    r#"version: '3'
services:
  androidtool:
    build: .
    devices:
      - /dev/bus/usb:/dev/bus/usb
    privileged: true"#
}
