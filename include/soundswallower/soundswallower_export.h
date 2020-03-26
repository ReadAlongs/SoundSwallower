#ifndef __SOUNDSWALLOWER_EXPORT_H__
#define __SOUNDSWALLOWER_EXPORT_H__

/* Win32 DLL gunk */
#if defined(_WIN32) && defined(SPHINX_DLL)
#if defined(SOUNDSWALLOWER_EXPORTS) /* DLL itself */
#define SOUNDSWALLOWER_EXPORT __declspec(dllexport)
#else
#define SOUNDSWALLOWER_EXPORT __declspec(dllimport)
#endif
#else /* No DLL things*/
#define SOUNDSWALLOWER_EXPORT
#endif

#endif /* __SOUNDSWALLOWER_EXPORT_H__ */
