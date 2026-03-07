# Incident Fix Plan — LIM-6

**What Failed:** HTTP request to retrieve index page at /
**Root Cause:** Missing route definition for the root URL '/' in the application
**Recommended Fix:** Add a catch-all route to handle requests for unknown routes at routes/index.js:

app.get('*', (req, res) => {
  res.status(404).send('Not Found');
});

## Evidence
- The provided codebase does not contain a route definition for the root URL '/' at routes/index.js
- The Express.js application is configured at server.js to use the routes defined in routes/index.js
- No catch-all route is defined to handle requests for unknown routes

Linear: https://linear.app/liminal-lodge/issue/LIM-6/kati-http-request-to-retrieve-index-page-at
