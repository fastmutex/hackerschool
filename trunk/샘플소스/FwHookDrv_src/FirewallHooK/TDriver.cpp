#include "stdafx.h"

#include "TDriver.h"


//Constructor. Inicializacion de variables.
TDriver::TDriver(void)
{
	driverHandle = NULL;
	   
	removable = TRUE;

	driverName = NULL;
	driverPath = NULL;
	driverDosName = NULL;

	initialized = FALSE;
	loaded = FALSE;
	started = FALSE;
}


//Destructor. Libera recursos.
TDriver::~TDriver(void)
{
	if(driverHandle != NULL)
	{
		CloseHandle(driverHandle); 
		driverHandle = NULL; 
	}
   
    UnloadDriver();
}

//Si removable = TRUE, no se descarga el driver en la "destruccion" del objeto
void TDriver::SetRemovable(BOOL value)
{
	removable = value;
}


// Esta inicializado el driver?
BOOL TDriver::IsInitialized(void)
{
	return initialized;
}

// Esta cargado el driver?
BOOL TDriver::IsLoaded(void)
{
	return loaded;
}

// Esta en estado Start el driver?
BOOL TDriver::IsStarted(void)
{
	return started;
}


// Inicia las variables del driver
DWORD TDriver::InitDriver(LPCTSTR path)
{
	// Si ya esta cargado primero lo descargo
	if(initialized)
	{
		if(UnloadDriver() != DRV_SUCCESS)
			return DRV_ERROR_ALREADY_INITIALIZED;
	}

	//  Reservo memoria para almacenar el path del driver
	driverPath = (LPTSTR)malloc(strlen(path) + 1);

	if(driverPath == NULL)
		return DRV_ERROR_MEMORY;

	strcpy(driverPath, path);


	// Voy a extraer el nombre del driver

	//Primero busco el caracter '\'
	LPTSTR sPos1 = strrchr(driverPath, (int)'\\');

	// Si no existe, me loco al principio
	if (sPos1 == NULL)
		sPos1 = driverPath;


	// Ahora busco la extension .sys
	LPTSTR sPos2 = strrchr(sPos1, (int)'.');

	// Si no tiene extension .sys salgo sin hacer nada mas
	if (sPos2 == NULL || sPos1 > sPos2)
	{
		free(driverPath);
		driverPath = NULL;

		return DRV_ERROR_INVALID_PATH_OR_FILE;
	}
	
	// Me quedo con el nombre del driver
	driverName = (LPTSTR) malloc (sPos2 - sPos1);
	
	if(driverName == NULL)
	{
		free(driverPath);
		driverPath = NULL;

		return DRV_ERROR_MEMORY;
	}

	// Copio los caracteres
	memcpy(driverName, sPos1 + 1, sPos2 - sPos1 - 1);
	
	driverName[sPos2 - sPos1 - 1] = 0;

	//driverDosName = \\.\driverName 
	driverDosName = (LPTSTR) malloc (strlen(driverName) + 5);

	if(driverDosName == NULL)
	{
		free(driverPath);
		driverPath = NULL;

		free(driverName);
		driverName = NULL;

		return DRV_ERROR_MEMORY;
	}

	sprintf(driverDosName, "\\\\.\\%s", driverName);

		
	initialized = TRUE;

	return DRV_SUCCESS;
}


// Inicia las variables del driver
DWORD TDriver::InitDriver(LPCTSTR name, LPCTSTR path, LPCTSTR dosName)
{	
	// Si ya esta cargado primero lo descargo
	if(initialized)
	{
		if(UnloadDriver() != DRV_SUCCESS)
			return DRV_ERROR_ALREADY_INITIALIZED;
	}

	LPTSTR dirBuffer;

	// Compruebo si el usuario introdujo un path
	if (path != NULL) 
	{
		// Lo copio en un buffer auxiliar para su posterior procesamiento
		DWORD len = (DWORD)(strlen(name) + strlen(path) + 1);
		dirBuffer = (LPTSTR) malloc (len);

		if(dirBuffer == NULL)
			return DRV_ERROR_MEMORY;

		strcpy(dirBuffer, path);

	}

	else 
	{
		// Si no tenemos path, supongo el directorio actual
		LPTSTR pathBuffer;
        DWORD len = GetCurrentDirectory(0, NULL);
      
		pathBuffer = (LPTSTR) malloc (len);

		if(pathBuffer == NULL)
			return DRV_ERROR_MEMORY;

		        
        if (GetCurrentDirectory(len, pathBuffer) != 0) 
		{
			len = (DWORD)(strlen(pathBuffer) + strlen(name) + 6);
			dirBuffer = (LPTSTR) malloc (len);

			if(dirBuffer == NULL)
			{
				free(pathBuffer);

				return DRV_ERROR_MEMORY;
			}

			// Path = directorio\driverName.sys
			sprintf(dirBuffer, "%s\\%s.sys", pathBuffer, name);

			// Compruebo si existe el fichero en cuestion
			if(GetFileAttributes(dirBuffer) == 0xFFFFFFFF)
			{
				free(pathBuffer);
				free(dirBuffer);

				// Si no existe, busco en system32\Drivers
				LPCTSTR sysDriver = "\\system32\\Drivers\\";
				LPTSTR sysPath;
	    	    
				DWORD len = GetWindowsDirectory(NULL, 0);
     			sysPath = (LPTSTR) malloc (len + strlen(sysDriver));

				if(sysPath == NULL)
					return DRV_ERROR_MEMORY;

				if (GetWindowsDirectory(sysPath, len) == 0) 
				{
					free(sysPath);
					
					return DRV_ERROR_UNKNOWN;
				}
	
				// Completo el path y compruebo si existe el fichero de nuevo
				strcat(sysPath, sysDriver);
				len = (DWORD)(strlen(sysPath) + strlen(name) + 5);

				dirBuffer = (LPTSTR) malloc (len);

				if(dirBuffer == NULL)
					return DRV_ERROR_MEMORY;

				sprintf(dirBuffer, "%s%s.sys", sysPath, name);

				free(sysPath);

				// Si el fichero no existe, salgo
				if(GetFileAttributes(dirBuffer) == 0xFFFFFFFF)
				{
					free(dirBuffer);

					return DRV_ERROR_INVALID_PATH_OR_FILE;
				}
			}

			free(pathBuffer);

        }

		else
		{
			free(pathBuffer);

			return DRV_ERROR_UNKNOWN;
		}
	}
	
	// Escribo las variables del driver
	driverPath = dirBuffer;

	driverName = (LPTSTR)malloc(strlen(name) + 1);

	if(driverName == NULL)
	{
		free(driverPath);
		driverPath = NULL;
		
		return DRV_ERROR_MEMORY;
	}

	strcpy(driverName, name);
	
	LPCTSTR auxBuffer;
	if(dosName != NULL)
        auxBuffer = dosName;
	
	else
		auxBuffer = name;

	//dosName=\\.\driverName
	if(auxBuffer[0] != '\\' && auxBuffer[1] != '\\')
	{
		driverDosName = (LPTSTR) malloc (strlen(auxBuffer) + 5);

		if(driverDosName == NULL)
		{
			free(driverPath);
			driverPath = NULL;

			free(driverName);
			driverName = NULL;

			return DRV_ERROR_MEMORY;
		}

		sprintf(driverDosName, "\\\\.\\%s", auxBuffer);
	}

	else
	{
		driverDosName = (LPTSTR) malloc (strlen(auxBuffer));

		if(driverDosName == NULL)
		{
			free(driverPath);
			driverPath = NULL;

			free(driverName);
			driverName = NULL;

			return DRV_ERROR_MEMORY;
		}

		strcpy(driverDosName, auxBuffer);
	}

	// Pongo el estado en inicializado
	initialized = TRUE;

	return DRV_SUCCESS;
}


// Funcion para cargar el driver.
DWORD TDriver::LoadDriver(LPCTSTR name, LPCTSTR path, LPCTSTR dosName, BOOL start)
{
	// Primero es necesario inicializarlo
	DWORD retCode = InitDriver(name, path, dosName);

	// Despues lo cargo
	if(retCode == DRV_SUCCESS)
		retCode = LoadDriver(start);

	return retCode;
}

// Funcion para cargar el driver
DWORD TDriver::LoadDriver(LPCTSTR path, BOOL start)
{
	// Primero lo inicializo
	DWORD retCode = InitDriver(path);

	// Despues lo cargo
	if(retCode == DRV_SUCCESS)
		retCode = LoadDriver(start);

	return retCode;
}


// Funcion para cargar el driver
DWORD TDriver::LoadDriver(BOOL start)
{
	// Si ya esta cargado no hago nada
	if(loaded)
		return DRV_SUCCESS;

	if(!initialized)
		return DRV_ERROR_NO_INITIALIZED;

	// Abro el Service Manager para crear el nuevo driver
	SC_HANDLE SCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	DWORD retCode = DRV_SUCCESS;
	
	if (SCManager == NULL) 
		return DRV_ERROR_SCM;

    

    // Creo el driver ("Servicio")
    SC_HANDLE  SCService = CreateService(SCManager,			  // SCManager database
									     driverName,            // nombre del servicio
							    		 driverName,            // nombre a mostrar
										 SERVICE_ALL_ACCESS,    // acceso total
										 SERVICE_KERNEL_DRIVER, // driver del kernel
										 SERVICE_DEMAND_START,  // comienzo bajo demanda
										 SERVICE_ERROR_NORMAL,  // control de errores normal
										 driverPath,	          // path del driver
										 NULL,                  // no pertenece a un grupo
										 NULL,                  // sin tag
										 NULL,                  // sin dependencias
										 NULL,                  // cuenta local del sistema
										 NULL                   // sin password
										 );
    
	// Si no puedo crearlo, miro si es porque ya fue cargado por otro proceso.
	if (SCService == NULL) 
	{
		SCService = OpenService(SCManager, driverName, SERVICE_ALL_ACCESS);
		
		if (SCService == NULL) 
			retCode = DRV_ERROR_SERVICE;
	}

    CloseServiceHandle(SCService);
	SCService=NULL;

	CloseServiceHandle(SCManager);
	SCManager = NULL;

	// Si todo fue bien, actualizo el estado e inicio si fuera necesario.
	if(retCode == DRV_SUCCESS)
	{
		loaded = TRUE;

		if(start)
			retCode = StartDriver();
	}

	return retCode;
}


// Funcion para descargar el driver.
DWORD TDriver::UnloadDriver(BOOL forceClearData)
{
	DWORD retCode = DRV_SUCCESS;

	// Si esta en estado started, primero lo paro
	if (started)
	{
		if ((retCode = StopDriver()) == DRV_SUCCESS) 
		{
			// Solo lo extraigo del service manager si es removable
			if(removable)
			{
				// Abro el servicio y elimino el driver
				SC_HANDLE SCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
				
				if (SCManager == NULL) 
					return DRV_ERROR_SCM;

				SC_HANDLE SCService = OpenService(SCManager, driverName, SERVICE_ALL_ACCESS);
				
				if (SCService != NULL)
				{
					if(!DeleteService(SCService))
						retCode = DRV_ERROR_REMOVING;
					else
						retCode = DRV_SUCCESS;
				}

				else
					retCode = DRV_ERROR_SERVICE;

				CloseServiceHandle(SCService);
				SCService = NULL;

				CloseServiceHandle(SCManager);
				SCManager = NULL;

				// Si todo fue bien, actualizo el estado
				if(retCode == DRV_SUCCESS)
					loaded = FALSE;
			}
		}
	}

	// Si el driver esta inicializado...
	if(initialized) 
	{
		// Si hubo algun problema pero esta a TRUE forceClear borro las variables de igual forma
		if(retCode != DRV_SUCCESS && forceClearData == FALSE)
			return retCode;
		
		// Actualizo el estado
		initialized = FALSE;
				
		// Libero memoria
		if(driverPath != NULL)
		{
			free(driverPath);
			driverPath = NULL;
		}


		if(driverDosName != NULL)
		{
			free(driverDosName);
			driverDosName = NULL;
		}

		if(driverName != NULL)
		{
			free(driverName);
			driverName = NULL;
		}

	}

	return retCode;
}



// Funcion para poner en Start al driver
DWORD TDriver::StartDriver(void)
{
	// Si ya esta comenzado, no hago nada
	if(started)
		return DRV_SUCCESS;

	// Abro el servicio y lo pongo en estado Start
	SC_HANDLE SCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	DWORD retCode;
	
	if (SCManager == NULL) 
		return DRV_ERROR_SCM;

    SC_HANDLE SCService = OpenService(SCManager,
		                              driverName,
				                      SERVICE_ALL_ACCESS);
    
	if (SCService == NULL) 
        return DRV_ERROR_SERVICE;

    
    if (!StartService( SCService, 0, NULL)) 
	{
		// Si el driver fue puesto en Start antes de mi, no lo elimino al destruir el objeto
		// debido a que fue creado por otro aplicacion que posiblemente lo este usando
        if (GetLastError() == ERROR_SERVICE_ALREADY_RUNNING) 
		{
			removable = FALSE;

			retCode = DRV_SUCCESS;
		}

		else
			retCode = DRV_ERROR_STARTING;
    }

	else
		retCode = DRV_SUCCESS;

  
    CloseServiceHandle(SCService);
	SCService = NULL;

	CloseServiceHandle(SCManager);
	SCManager = NULL;

	// Actualizo el estado y abro el dispositivo para comunicacion Usuario -> Driver
	if(retCode == DRV_SUCCESS)
	{
		started = TRUE;

		retCode = OpenDevice();
	}

    return retCode;
}



// Funcion para parar el driver
DWORD TDriver::StopDriver(void)
{
	// Si ya esta parado, no tengo que hacer nada
	if(!started)
		return DRV_SUCCESS;

	// Cambio el estado del proceso
	SC_HANDLE SCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	DWORD retCode;
	
	if (SCManager == NULL) 
		return DRV_ERROR_SCM;

   
    SERVICE_STATUS  status;

    SC_HANDLE SCService = OpenService(SCManager, driverName, SERVICE_ALL_ACCESS);
    
	if (SCService != NULL)
	{
		// Cierro tambien el handle al driver abierto en funcion start
		CloseHandle(driverHandle); 
		driverHandle = NULL; 

		if(!ControlService(SCService, SERVICE_CONTROL_STOP, &status))
			retCode = DRV_ERROR_STOPPING;

		else
			retCode = DRV_SUCCESS;
	}

	else
		retCode = DRV_ERROR_SERVICE;


    CloseServiceHandle(SCService);
	SCService = NULL;

	CloseServiceHandle(SCManager);
	SCManager = NULL;

	// Actualizo el estado
	if(retCode == DRV_SUCCESS)
		started = FALSE;

    return retCode;
}


// Funcion para abrir el dispositivo
DWORD TDriver::OpenDevice(void)
{
	// Si ya tengo un handle al driver, primero lo cierro
	if (driverHandle != NULL) 
		CloseHandle(driverHandle);

    driverHandle = CreateFile(driverDosName,
							  GENERIC_READ | GENERIC_WRITE,
							  0,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);


    if(driverHandle == INVALID_HANDLE_VALUE)
		return DRV_ERROR_INVALID_HANDLE;
	
	return DRV_SUCCESS;
}


// Devuelve un handle al driver
HANDLE TDriver::GetDriverHandle(void)
{
	return driverHandle;
}


// Funcion para enviar datos al driver
DWORD TDriver::WriteIo(DWORD code, PVOID buffer, DWORD count)
{
	if(driverHandle == NULL)
		return DRV_ERROR_INVALID_HANDLE;

	DWORD bytesReturned;

	BOOL returnCode = DeviceIoControl(driverHandle,
								      code,
								      buffer,
								      count,
								      NULL,
								      0,
								      &bytesReturned,
								      NULL);

	if(!returnCode)
		return DRV_ERROR_IO;

	return DRV_SUCCESS;
}


// Funcion para leer datos del driver
DWORD TDriver::ReadIo(DWORD code, PVOID buffer, DWORD count)
{
	if(driverHandle == NULL)
		return DRV_ERROR_INVALID_HANDLE;

	DWORD bytesReturned;
	BOOL retCode = DeviceIoControl(driverHandle,
								   code,
								   NULL,
								   0,
								   buffer,
								   count,
								   &bytesReturned,
								   NULL);

	if(!retCode)
		return DRV_ERROR_IO;

	return bytesReturned;
}


// Funcion para realizar operacion de E/S con el driver (leer y/o escribir)
DWORD TDriver::RawIo(DWORD code, PVOID inBuffer, DWORD inCount, PVOID outBuffer, DWORD outCount)
{
	if(driverHandle == NULL)
		return DRV_ERROR_INVALID_HANDLE;

	DWORD bytesReturned;
	BOOL retCode = DeviceIoControl(driverHandle,
								   code,
								   inBuffer,
								   inCount,
								   outBuffer,
								   outCount,
								   &bytesReturned,
								   NULL);

	if(!retCode)
		return DRV_ERROR_IO;

	return bytesReturned;
}