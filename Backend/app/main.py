from fastapi import FastAPI
from app.database import Base, engine
from app.routers import employe
from fastapi.middleware.cors import CORSMiddleware


app = FastAPI()

Base.metadata.create_all(bind=engine)

origins = ["http://localhost:4028"]

app.add_middleware(
    CORSMiddleware,
    allow_origins=origins,        # Autorise ces origines spécifiques
    allow_credentials=True,       # Autorise l'envoi de cookies ou d'identifiants
    allow_methods=["*"],          # Autorise toutes les méthodes (GET, POST, PUT, etc.)
    allow_headers=["*"],          # Autorise tous les en-têtes
)

app.include_router(employe.router)