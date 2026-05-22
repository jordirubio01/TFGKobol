# **Kobol - VST Plugin** 🎛️  

## Modeling the RSF Kobol Expander

This repository is part of the Bachelor's Thesis project *Digital Virtualization of VCO, VCF, and VCA Modules in the RSF Kobol Synthesizer*, carried out by Jordi Rubio Arbona and supervised by Xavier Lizarraga, at **Universitat Pompeu Fabra** in Barcelona.

## The repository contains

* 🎛️ The **VST plugin** and **standalone application** developed using the JUCE framework.
* 📊 Python notebooks

## Installation

### Windows

Two versions are available in the [`Plugin/Windows`](https://github.com/jordirubio01/TFGKobol/tree/19a02396b5f43a9df74f52762f8ec90f7e39004b/Plugin/Windows) folder:

- [`Kobol Plugin.exe`](https://github.com/jordirubio01/TFGKobol/blob/19a02396b5f43a9df74f52762f8ec90f7e39004b/Plugin/Windows/Standalone/Kobol%20Plugin.exe)
  Standalone executable version. No installation is required — simply download and run the `.exe` file.

- [`Kobol Plugin.vst3`](https://github.com/jordirubio01/TFGKobol/blob/19a02396b5f43a9df74f52762f8ec90f7e39004b/Plugin/Windows/VST3/Kobol%20Plugin.vst3)
  VST3 plugin version for DAWs.

To install the VST3 plugin:

1. Download [`Kobol Plugin.vst3`](https://github.com/jordirubio01/TFGKobol/blob/19a02396b5f43a9df74f52762f8ec90f7e39004b/Plugin/Windows/VST3/Kobol%20Plugin.vst3)
2. Copy the file to:

```text
C:\Program Files\Common Files\VST3
```

3. Restart your DAW and rescan plugins if necessary.

### MacOS

Two versions are available in the Plugin/OS folder:
- `Kobol Plugin.app`
  Standalone application version. No installation is required — simply download and run the `.exe` file.
- `Kobol Plugin.vst3`
  VST3 plugin version for DAWs.

To install the VST3 plugin:

1. Download `Kobol Plugin.vst3`
2. Copy the file to:
```text
/Library/Audio/Plug-Ins/VST3
```
3. Restart your DAW and rescan plugins if necessary.

## Acknowledgements

The initial code was taken from the https://github.com/Hardkoala/KobolVCO repository, developed as part of a master's thesis at the **Universitat Pompeu Fabra** by Valentín Malpica Gómez under the MIT license (see LICENSE), has been extended with new components for VCA and VCF.

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.13862935.svg)](https://doi.org/10.5281/zenodo.13862935)

```bibtex
@misc{malpica2024kobolvco,
  author       = {Valentín Malpica Gómez},
  title        = {Virtualization of the Behringer Kobol Synthesizer VCO},
  year         = {2024},
  publisher    = {Zenodo},
  doi          = {10.5281/zenodo.13862935},
  url          = {https://doi.org/10.5281/zenodo.13862935}
}
```

## Contributing

If you'd like to contribute to this project, feel free to fork the repository and submit a pull request. Contributions are welcome!

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
