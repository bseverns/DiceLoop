# Footswitch Adapter Notes

Pedal builds use beefy stomp switches. Here's how to integrate them without wrecking the control logic.

## Mechanical Adapter
- 3D print a 5 mm thick spacer to center the stomp switch in a 1590XX enclosure. Document STL revisions here.
- Add a neoprene washer under the panel to kill clacks—log thickness so others can repeat.

## Electrical Considerations
- Stomp switches are latching by default. Either buy momentary versions or rework the firmware to detect state changes. If you patch the code, link to the commit here.
- Parallel a 1 µF cap for extra debounce if your firmware tolerates the slower release.

## Cable Dress
- Route the footswitch wires away from the audio jack to dodge switching pops. Twist the leads and anchor with hot glue if the rig will tour.

_Throw in photos or updated measurements as you iterate._
