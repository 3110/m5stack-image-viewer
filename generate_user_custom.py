Import("env")
import os
import re

firmware_dir = env.GetProjectOption('custom_firmware_dir')
if not os.path.exists(firmware_dir):
    os.makedirs(firmware_dir)

firmware_version_file = env.GetProjectOption("custom_firmware_version_file")
with open(os.path.join(env.subst("$PROJECT_SRC_DIR"), firmware_version_file), 'r') as f:
    for line in f:
       match = re.search(r'\"v(\d+\.\d+\.\d+)\"', line)
       if match:
            firmware_version = match.group(1)
            break
    else:
        raise RuntimeError('Version is not found in %s' % version_file)

firmware_target = env.GetProjectOption("custom_firmware_target")
firmware_name = env.GetProjectOption("custom_firmware_name")
firmware_suffix = env.GetProjectOption('custom_firmware_suffix')

firmware_bin = '%s_%s_firmware_%s.%s' % (firmware_target, firmware_name, firmware_version, firmware_suffix)
firmware_path = os.path.join(firmware_dir, firmware_bin)

app_bin = os.path.join('$BUILD_DIR','${PROGNAME}.%s' % firmware_suffix)

env.AddCustomTarget(
    name="firmware",
    dependencies=['buildfs'],
    actions=[
        '"$PYTHONEXE" "$UPLOADER" --chip esp32 merge_bin '
        '-o %s %s $ESP32_APP_OFFSET %s'
        % (firmware_path, ' '.join([
            addr + " " + name for addr, name in env['FLASH_EXTRA_IMAGES']
        ]), app_bin)
    ],
    title="Generate User Custom")
