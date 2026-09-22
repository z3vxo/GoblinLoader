# self hosted server


1. on startup take in user creds
2. collect machine identifiers
3. POST to our domain /login with creds -> returns very short lived JWT
4. POST to our domain /register with jwt and machine data -> server verfiys subscription, verfiys not already registered etc -> returns long lived signed token used for protected routes



routes for self hosted
/api/login -> usual login route, used after setup, just checks local DB for users




GET /api/campiagns -> list campaigns, 
POST /api/campiagns -> create campaing 
DELETE /api/campiagns -> delete campaing
PATCH /api/campiagn -> modify info about campaign

these 2 below together
/api/file/upload -> upload payload also sends campaing ID to easily map them, server generates uuid, checks file type, if exe checks for .reloc etc stores into DB(uuid, file path, file type, has .reloc etc)
returns uuid, 
/api/build -> sends output type, file uuid, load and exit vs load and listen etc, proxys to our backend /internal/build -> server also returns agent id(server maps to campaing id) returns chosen output



GET /api/hosts?id=<campaing id> -> return all hosts for specific campaing 
POST /api/host/sendtask -> send task to specific agent using agent id



so the flow is
1. user signs in
2. presses on campaigns
3. presses on whatever campaign
4. loads to /campaing/<uuid> which pulls hosts
5. can press "files" -> shows files for campaing
6. can press "builds" -> list builds for campaings(links file)