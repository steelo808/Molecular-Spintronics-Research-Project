@set host=localhost
@set port=8080
@java -Xmx6G -cp bin msd_server.MSDServer %host% %port%
