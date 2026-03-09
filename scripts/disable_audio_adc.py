from SCons.Script import Import

Import("env")


def skip_input_adc(env, node):
    # The Teensy 4.x line-in path uses the audio shield codec, not MCU ADC input.
    # Skipping this source avoids building the Kinetis-centric ADC module.
    path = node.srcnode().get_path().replace("\\", "/")
    if path.endswith("/libraries/Audio/input_adc.cpp"):
        return None
    return node


env.AddBuildMiddleware(skip_input_adc, pattern="*/libraries/Audio/input_adc.cpp")
