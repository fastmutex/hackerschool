
#if !defined(TDRIVER_CLASS)
#define TDRIVER_CLASS

#pragma once

#include "winsvc.h"


//ERROR CODES
#define DRV_SUCCESS						 (DWORD)0		// Todo bien

#define DRV_ERROR_SCM					 (DWORD)-1		// ERROR al abrir el service manager
#define DRV_ERROR_SERVICE				 (DWORD)-2		// ERROR al crear el servicio
#define DRV_ERROR_MEMORY				 (DWORD)-3		// ERROR al reservar memoria
#define DRV_ERROR_INVALID_PATH_OR_FILE	 (DWORD)-4		// ERROR, Path no valido
#define DRV_ERROR_INVALID_HANDLE		 (DWORD)-5		// ERROR, driver handle no valido
#define DRV_ERROR_STARTING				 (DWORD)-6		// ERROR al poner en Start el driver
#define DRV_ERROR_STOPPING				 (DWORD)-7		// ERROR al parar el driver
#define DRV_ERROR_REMOVING				 (DWORD)-8		// ERROR eliminando el "servicio"
#define DRV_ERROR_IO					 (DWORD)-9		// ERROR en operacion de E/S
#define DRV_ERROR_NO_INITIALIZED		 (DWORD)-10		// ERROR, clase no inicializada
#define DRV_ERROR_ALREADY_INITIALIZED	 (DWORD)-11		// ERROR, clase ya inicializada
#define DRV_ERROR_NULL_POINTER			 (DWORD)-12		// ERROR, puntero a null como parametro
#define DRV_ERROR_UNKNOWN				 (DWORD)-13		// ERROR desconocido



class TDriver
{
public:
	TDriver(void);		//constructor
	~TDriver(void);		//destructor

	// Funciones para inicializar las variables del driver
	DWORD InitDriver(LPCTSTR name, LPCTSTR path, LPCTSTR dosName=NULL);
	DWORD InitDriver(LPCTSTR path);


	// Funciones para carga/descarga del driver. Si start = TRUE, el driver sera puesto en estado Start.
	DWORD LoadDriver(BOOL start = TRUE);
	DWORD LoadDriver(LPCTSTR name, LPCTSTR path, LPCTSTR dosName=NULL, BOOL start=TRUE);
	DWORD LoadDriver(LPCTSTR path, BOOL start=TRUE);

	// Si forceClearData == TRUE, las variables seran eliminadas aunque no podamos eliminar el servicio
	DWORD UnloadDriver(BOOL forceClearData = FALSE);
	
	// Funciones parar comenzar/Parar el servicio
	DWORD StartDriver(void);
	DWORD StopDriver(void);

	// Si vale TRUE, el driver sera eliminado en el destructor
	void SetRemovable(BOOL value);


	// Informacion de estado del driver
	BOOL IsInitialized();
	BOOL IsStarted();
	BOOL IsLoaded();


	// Funcion para obtener un handle al driver
	HANDLE GetDriverHandle(void);

	// Funciones para realizar operaciones de E/S con el driver
	DWORD WriteIo(DWORD code, PVOID buffer, DWORD count);
	DWORD ReadIo(DWORD code, PVOID buffer, DWORD count);
	DWORD RawIo(DWORD code, PVOID inBuffer, DWORD inCount, PVOID outBuffer, DWORD outCount);
	
private:
	HANDLE driverHandle;	// driver handle
	
	LPTSTR driverName;		// Nombre del driver
	LPTSTR driverPath;		// Path del driver
	LPTSTR driverDosName;	// Nombre DOS del driver

	BOOL initialized;		// Variables donde almacenar el estado del driver
	BOOL started;
	BOOL loaded;
	BOOL removable;

	DWORD OpenDevice(void);	// Obtiene un handle al driver	
};

#endif