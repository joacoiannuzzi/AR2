## Base de proyecto
### Instrucciones
* Agregar glfw a la carpeta libraries, como ya esta agregado como submodulo se puede traer del repositorio de git con 
```
git submodule init
git submodule update
```
* Agregar GLAD (ver el tutorial para obtenerlo)
una vez que tengan el zip de GLAD descomprimanlo en una carpeta ./libraries/glad

* Agregar su codigo al archivo main.cpp

### Compilado
Para compilar primero hay que generar el makefile con CMake, parados en la carpeta raiz del proyecto (donde esta el archivo CMakeLists.txt
```
cmake .
```
Una vez generado el makefile se puede ejecutar el mismo con el comando make

```
make
```

El ejecutable va a estar en la misma carpeta que estan y va a tener el nombre del proyecto (configurado en CMakeLists.txt, por defecto 'introcom')

Para ejecutar el programa

```
./introcom
```

