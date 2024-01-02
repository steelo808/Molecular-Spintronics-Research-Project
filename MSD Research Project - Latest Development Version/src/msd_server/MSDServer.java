package msd_server;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.InetSocketAddress;
import java.net.URLDecoder;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.Executors;

import com.sun.net.httpserver.HttpServer;

public class MSDServer {
	public static final String PROTOCOL = "http";
	public static final InetSocketAddress ADDRESS = new InetSocketAddress("localhost", 8080);
	public static final String CHARSET = StandardCharsets.UTF_8.name();

	public static String decode(String str) {
		try {
			return URLDecoder.decode(str, CHARSET);
		} catch(UnsupportedEncodingException ex) {
			throw new Error(ex);  // This shouldn't be able to happen!
		}
	}

	public static WorkerPool workers = new WorkerPool();

	public static void main(String[] args) throws IOException {
		HttpServer server = HttpServer.create(ADDRESS, 0);
		server.setExecutor(Executors.newCachedThreadPool());
		server.createContext("/msd", new MSDHttpHandler() {
			@Override
			public void handle(HttpRequest req, HttpResponse res) throws IOException {
				switch(req.method) {
				case GET: {
					/*
					* Summary: Check the state of the given MSDWorker.
					* 
					* Takes query-param id=[MSD ID]
					* Returns state of the MSD Worker.
					*/
					MSDWorker msd = workers.lookup(req, res);
					if (msd == null)  break;

					res.setBody(msd.getState());
					HttpHeader.CONTENT_TYPE.to("application/json", res.headers);
					res.status = HttpStatus.OK;
				} break;
				
				case POST: {
					/*
					* Summary: Create a new MSDWorker.
					* 
					* Takes JSON containing MSD.init (required) and
					* 	MSD.parameters (optional) values
					* Returns MSD ID for the newly created MSDWorker.
					*/
					String id = workers.createWorker(req.getBody()).toString();
					res.setBody(String.format("{\"id\":%s}", id));
					HttpHeader.CONTENT_TYPE.to("application/json", res.headers);
					HttpHeader.LOCATION.to( String.format("%s://%s:%s/msd?id=%s",
						PROTOCOL, ADDRESS.getHostString(), ADDRESS.getPort(), id
						), res.headers );
					res.status = HttpStatus.CREATED;
				} break;
				
				case PATCH: {
					/*
					* Summary: Change MSD.parameters for the given MSDWorker.
					* 
					* Takes query-param id=[MSD ID].
					* Takes JSON containing MSD.parameter values to update.
					* Returns empty JSON.
					*/
					MSDWorker msd = workers.lookup(req, res);
					if (msd == null)  break;

					msd.setParameters(req.getBody());
					res.status = HttpStatus.OK;
				} break;
				
				case DELETE: {
					/*
					* Summary: Destroy the given MSDWorker.
					* 
					* Takes query-param id=[MSD ID].
					* Returns empty HTTP body.
					*/
					MSDWorker msd = workers.destoryWorker(req, res);
					if (msd == null)  break;  // don't set HTTP status to 200 OK

					res.status = HttpStatus.OK;
				} break;
				
				default:
					res.status = HttpStatus.METHOD_NOT_ALLOWED;
				}
			}
		});  // get, post, patch, and delete
		
		// TODO: server.createContext("/msd/run", null);  // post
		
		/*
		server.createContext("/msd/create", new MSDHttpHandler() {
			int count = 0;

			@Override
			public void handle(HttpRequest request, HttpResponse response) throws Exception {
				// TODO: DEBUG
				// Runtime.getRuntime().exec("cls");
				System.out.println("---- [Request " + (++count) + "] ----");
				System.out.print(request.method + " " + request.path);
				if (request.query != null)
					System.out.print("?" + request.query);
				if (request.fragment != null)
					System.out.print("#" + request.fragment);
				System.out.println();
				for (Map.Entry<String, List<String>> field : request.headers.entrySet())
					System.out.println(field.getKey() + "=" + field.getValue());
				if (request.body != null)
					System.out.println(request.body);
				System.out.println("------------------------------------------------------------");
				response.status = 200;
				HttpHeader.CONTENT_TYPE.to("text/plain; charset=" + CHARSET, response.headers);
				response.setBody("Hello, world!");
			}
		});
		*/

		server.start();
		System.out.println("Server started in Thread: " + server.getExecutor());
	}
}