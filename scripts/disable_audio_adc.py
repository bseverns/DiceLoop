from pathlib import Path
from SCons.Script import Import

Import("env")


def disable_audio_input_adc(target, source, env):
    project_libdeps = Path(env.subst("$PROJECT_LIBDEPS_DIR"))
    active_env = env.subst("$PIOENV")
    audio_dir = project_libdeps / active_env / "Audio"
    if not audio_dir.exists():
        return

    adc_impl = audio_dir / "input_adc.cpp"
    if not adc_impl.exists():
        return

    backup = audio_dir / "input_adc.cpp.disabled"
    try:
        adc_impl.rename(backup)
    except OSError:
        pass


env.AddPreAction("buildprog", disable_audio_input_adc)
