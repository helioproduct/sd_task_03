FROM postgres:16-alpine

COPY schema.sql /docker-entrypoint-initdb.d/01-schema.sql
COPY data.sql /docker-entrypoint-initdb.d/02-data.sql
