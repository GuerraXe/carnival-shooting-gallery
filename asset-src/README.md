# asset-src/ (not shipped)

This folder holds the masters and originals behind the runtime files in `../assets/`. See [`../docs/ASSET_GUIDE.md`](../docs/ASSET_GUIDE.md) §5 and decision D-021.

- `originals/<category>/`: untouched downloads and camera RAW files. Never edited. Each one has its SHA-256 recorded in the manifest.
- `work/<category>/`: layered edit masters (.xcf, .kra, .psd, or .tif; 16-bit).
- `reference/`: mood and reference images only. Never exported to runtime.

The storage method for large files (Git LFS, external storage, or don't keep) is still open (Asset Guide Q-4).
