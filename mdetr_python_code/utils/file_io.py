from iopath.common.file_io import HTTPURLHandler, PathManager


def _get_path_manager() -> PathManager:
    try:
        from torchmultimodal.fb.utils.file_io import FBPathManager, register_handlers

        pm = FBPathManager()
        register_handlers(pm)
        return pm
    except ImportError:
        pm = PathManager()
        pm.register_handler(HTTPURLHandler())
        return pm
