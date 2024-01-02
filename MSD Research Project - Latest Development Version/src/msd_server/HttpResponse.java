package msd_server;

import static msd_server.MSDServer.CHARSET;

import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.io.UnsupportedEncodingException;

import com.sun.net.httpserver.Headers;
import com.sun.net.httpserver.HttpExchange;

/**
 * @author Christopher D'Angelo
 * @version December 29, 2023
 */
public class HttpResponse {
	private HttpExchange exchange;

	public HttpStatus status;
	public Headers headers;
	public byte[] body = null;

	public HttpResponse(HttpExchange exchange) {
		this.exchange = exchange;
		this.headers = exchange.getResponseHeaders();
		// TODO: Set some default headers?
		// HttpHeader.DATE.to(new Date().toGMTString(), response.headers);
	}

	public void setBody(String body) {
		try {
			this.body = body.getBytes(CHARSET);
		} catch(UnsupportedEncodingException ex) {
			throw new Error();  // This should never happen!
		}
	}

	public void setException(Throwable ex) {
		status = HttpStatus.INTERNAL_SERVER_ERROR;
		StringWriter trace = new StringWriter();
		ex.printStackTrace(new PrintWriter(trace));
		setBody(String.format("""
			<html>
				<body>
					<p>%s</p> <br>
					<pre>%s</pre>
				</body>
			</html>
		""", status, trace));
		HttpHeader.CONTENT_TYPE.to("text/html; utf-8", headers);
	}

	public void send() throws IOException {
		if (body != null) {
			exchange.sendResponseHeaders(status.code, body.length);
			exchange.getResponseBody().write(body);
		} else {
			exchange.sendResponseHeaders(status.code, -1);
		}
	}
}
