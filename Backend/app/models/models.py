from sqlalchemy import Column, String, Date, Boolean, Text, ForeignKey, Numeric, DateTime, Integer
from sqlalchemy.dialects.postgresql import UUID
from sqlalchemy.sql import func
from sqlalchemy.orm import relationship
import uuid
from app.database import Base
from datetime import datetime

class Employe(Base):
    __tablename__ = "employes"

    id = Column(UUID(as_uuid=True), primary_key=True, default=uuid.uuid4)
    company_id = Column(String, default = "e0ce87c8-73aa-4396-9dd5-2e078b3a9722", nullable=True)
    full_name = Column(String, nullable=False)
    email = Column(String, unique=True, nullable=False)
    phone = Column(String, nullable=True)
    date_of_birth = Column(Date, nullable=True)
    address = Column(String, nullable=True)
    gender = Column(String, nullable=True)
    marital_status = Column(String, nullable=True)

    employee_code = Column(String, unique=True, nullable=True)
    code_unique = Column(String, unique=True, nullable=False)
    position = Column(String, nullable=True)
    department = Column(String, nullable=True)
    hire_date = Column(Date, nullable=True)
    contract_type = Column(String, nullable=True)
    salary = Column(Numeric, nullable=True)
    contract_end_date = Column(Date, nullable=True)
    work_start_time = Column(String, nullable=True)
    work_end_time = Column(String, nullable=True)

    education_level = Column(String, nullable=True)
    experience = Column(String, nullable=True)

    skip_biometric = Column(Boolean, default=False, nullable=True)
    fingerprints_enrolled = Column(Boolean, default=False, nullable=True)

    actif = Column(Boolean, default=True)
    created_at = Column(DateTime(timezone=True), server_default=func.now())
    
    # Relations
    presences = relationship("Presence", back_populates="employe", cascade="all, delete-orphan")
    empreintes = relationship("Empreinte", back_populates="employe", cascade="all, delete-orphan")


class Empreinte(Base):
    __tablename__ = "empreintes"

    id = Column(UUID(as_uuid=True), primary_key=True, default=uuid.uuid4)
    employe_id = Column(UUID(as_uuid=True), ForeignKey("employes.id"), nullable=False)
    empreinte_id = Column(Integer, nullable=False)  # 1, 2, 3, etc.
    created_at = Column(DateTime(timezone=True), server_default=func.now())
    
    # Relation
    employe = relationship("Employe", back_populates="empreintes")


class Presence(Base):
    __tablename__ = "presences"

    id = Column(UUID(as_uuid=True), primary_key=True, default=uuid.uuid4)
    employe_id = Column(UUID(as_uuid=True), ForeignKey("employes.id"), nullable=False)
    employee_code = Column(String, nullable=False)
    date_presence = Column(Date, nullable=False)
    heure_arrivee = Column(DateTime, nullable=False)
    heure_depart = Column(DateTime, nullable=True)
    type_presence = Column(String, default="entrée")  # entrée, sortie, pause
    notes = Column(Text, nullable=True)
    created_at = Column(DateTime(timezone=True), server_default=func.now())

    # Relation
    employe = relationship("Employe", back_populates="presences")
