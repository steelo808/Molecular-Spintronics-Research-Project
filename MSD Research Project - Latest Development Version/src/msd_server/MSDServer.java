package msd_server;

import static msd_server.HttpStatus.OK;
import static msd_server.HttpStatus.CREATED;
import static msd_server.HttpStatus.ACCEPTED;
import static msd_server.HttpStatus.NOT_FOUND;
import static msd_server.HttpStatus.METHOD_NOT_ALLOWED;

import static msd_server.HttpHeader.CONTENT_TYPE;
import static msd_server.HttpHeader.LOCATION;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.InetSocketAddress;
import java.net.URLDecoder;
import java.nio.charset.StandardCharsets;
import java.util.List;
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

	public static final WorkerPool workers = new WorkerPool();

	/**
	 * Path: /msd
	 * Methods: POST, PATCH, GET, DELETE
	 */
	private static final MSDHttpHandler msdHandler = (req, res) -> {
		switch(req.method) {
		case POST: {
			/*
			* Summary: Create a new MSDWorker.
			* 
			* Takes JSON containing MSD.init (required) and
			* 	MSD.parameters (optional) values
			* Returns MSD ID for the newly created MSDWorker.
			*/
			String id = workers.createWorker(req.getBody()).toString();
			res.setBody(String.format("{\"id\":\"%s\"}", id));
			CONTENT_TYPE.to("application/json", res.headers);
			LOCATION.to( String.format("%s://%s:%s/msd?id=%s",
				PROTOCOL, ADDRESS.getHostString(), ADDRESS.getPort(), id
				), res.headers );
			res.status = CREATED;
		} break;

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
			CONTENT_TYPE.to("application/json", res.headers);
			res.status = OK;
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
			res.status = OK;
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

			res.status = OK;
		} break;
		
		default:
			res.status = METHOD_NOT_ALLOWED;
		}
	};

	/**
	 * Path: /msd/run
	 * Methods: POST, DELETE
	 */
	private static final MSDHttpHandler runHandler = (req, res) -> {
		switch(req.method) {
		case POST: {
			/*
			 * Summary: Runs the metropolis algorith.
			 * 
			 * HTTP Request body:
			 * {
			 * 	"simCount": int,  // (Required) The number of iterations to simulate
			 *  "freq": int,      // (Optional) how frequently to record the MSD state, measured in iterations
			 *  "dkT": double,    // (Optional) Heat or cool the system linearly at the given rate (per 1 iteration)
			 *  "dB": Vector      // (Optional) Change the magnetic field linearly at the given rete (per 1 iteration)
			 * }
			 */
			MSDWorker msd = workers.lookup(req, res);
			if (msd == null)  break;
			
			msd.run(req.getBody());
			LOCATION.to( String.format("%s://%s:%s/msd/record?id=%s",
				PROTOCOL, ADDRESS.getHostString(), ADDRESS.getPort(), req.query.get("id")
				), res.headers );
			res.status = ACCEPTED;
		} break;

		case DELETE: {
			/*
			 * Summary: Cancel the simualtion if it is currently running.
			 */
			MSDWorker msd = workers.lookup(req, res);
			if (msd == null)  break;

			msd.cancel();
			res.status = OK;
		} break;

		default:
			res.status = METHOD_NOT_ALLOWED;
		}
	};

	/**
	 * Path: msd/record
	 * Method: GET
	 */
	private static final MSDHttpHandler recordHandler = (req, res) -> {
		switch(req.method) {
		case GET: {
			MSDWorker msd = workers.lookup(req, res);
			if (msd == null)  break;

			List<String> record = msd.getRecord();
			int length = record.size();
			if (req.query.containsKey("all")) {
				int length = ; // TODO: calculate size of StringBuilder...
				// TODO: Construct a parent JSON object containing all of the states and send.

			} else if (req.query.containsKey("index")) {
				try {
					int index = Integer.valueOf(req.query.get("index"));
					if (index < 0)  index = index + length;
					res.setBody(record.get(index));
				} catch(IndexOutOfBoundsException | NumberFormatException ex) {
					res.status = NOT_FOUND;
					break;
				}
			} else {
				res.setBody(String.format("{\"length\":%s}", length));
			}
			CONTENT_TYPE.to("application/json", res.headers);
			res.status = OK;
		} break;

		default:
			res.status = METHOD_NOT_ALLOWED;
		}
	};

	public static void main(String[] args) throws IOException {
		HttpServer server = HttpServer.create(ADDRESS, 0);
		server.setExecutor(Executors.newCachedThreadPool());

		server.createContext("/msd", msdHandler);
		
		// TODO: server.createContext("/msd/run", null);
			
		server.start();
		System.out.println("Server started in Thread: " + server.getExecutor());  // DEBUG
	}
}