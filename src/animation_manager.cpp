
#include "animation_manager.h"

template <class FaceType>
void Jacobian<FaceType>::FromUV( CoordType p0,CoordType p1,CoordType p2,
                    Point2X u0,Point2X u1,Point2X u2,
                    CoordType &u,CoordType &v)
{
    p1-=p0;
    p2-=p0;
    u1-=u0;
    u2-=u0;

    ScalarType det = u1 ^ u2;

    u1/=det;
    u2/=det;

    u = p1*u2.Y() - p2*u1.Y();
    v = p2*u1.X() - p1*u2.X();
}


template <class FaceType>
void Jacobian<FaceType>::From3DTris(CoordType p0,CoordType p1,CoordType p2,
                        CoordType p0_def,CoordType p1_def,CoordType p2_def,
                        CoordType &u,CoordType &v)
{
    //retrieve UV Coords
    CoordType bary=(p0_def + p1_def + p2_def)/3;
    p0_def-=bary;
    p1_def-=bary;
    p2_def-=bary;

    CoordType N_source=(p1_def-p0_def)^(p2_def-p0_def);
    CoordType N_dest(0,0,1);
    N_source.Normalize();
    N_dest.Normalize();

    vcg::Matrix33<ScalarType> RotM=vcg::RotationMatrix(N_source,N_dest);
    p0_def=RotM*p0_def;
    p1_def=RotM*p1_def;
    p2_def=RotM*p2_def;

    assert(fabs(p0_def.Z())<0.0001);
    assert(fabs(p1_def.Z())<0.0001);
    assert(fabs(p2_def.Z())<0.0001);

    Point2X UV0(p0_def.X(),p0_def.Y());
    Point2X UV1(p1_def.X(),p1_def.Y());
    Point2X UV2(p2_def.X(),p2_def.Y());

    FromUV(p0,p1,p2,UV0,UV1,UV2,u,v);
}

template <class FaceType>
void Jacobian<FaceType>::FromFace(const FaceType &F,CoordType &u,CoordType &v)
{
    const VertexType *v0=F.cV(0);
    const VertexType *v1=F.cV(1);
    const VertexType *v2=F.cV(2);

    CoordType P0=v0->P();
    CoordType P1=v1->P();
    CoordType P2=v2->P();

    CoordType RPos0=v0->RPos;
    CoordType RPos1=v1->RPos;
    CoordType RPos2=v2->RPos;

    From3DTris(P0,P1,P2,RPos0,RPos1,RPos2,u,v);
}

template <class TriMeshType>
void AnimationManager<TriMeshType>::InterpolateFaceField(const size_t &IndexFace,const size_t &IndexFrame,
                            CoordType &InterpCurvDirection,
                            ScalarType &InterpAnisotropyVal)
{
    assert(IndexFace<target_shape.face.size());
    assert(IndexFrame<NumFrames());
    assert(IndexFrame<JU.size());
    assert(IndexFrame<JV.size());
    assert(IndexFace<target_shape.face.size());

    assert(IndexFace<FaceFaceIdx.size());
    size_t IndexFAnim=FaceFaceIdx[IndexFace];
    InterpCurvDirection=PerFrameCurvVect[IndexFrame][IndexFAnim];
    InterpAnisotropyVal=PerFrameCurvAnis[IndexFrame][IndexFAnim];
}

template <class TriMeshType>
void AnimationManager<TriMeshType>::InterpolateFaceStretch(const size_t &IndexFace,const size_t &IndexFrame,
                            CoordType &FaceJU,CoordType &FaceJV)
{
    assert(IndexFrame<NumFrames());
    assert(IndexFrame<JU.size());
    assert(IndexFrame<JV.size());

    assert(IndexFace<target_shape.face.size());

    assert(IndexFace<FaceFaceIdx.size());
    size_t IndexFAnim=FaceFaceIdx[IndexFace];

    FaceJU=JU[IndexFrame][IndexFAnim];
    FaceJV=JV[IndexFrame][IndexFAnim];
}

template <class TriMeshType>
typename TriMeshType::CoordType AnimationManager<TriMeshType>::InterpolatePos(size_t IndexV,size_t IndexFrame)
{
    assert(IndexV<target_shape.vert.size());
    assert(IndexFrame<PerFramePos.size());

    size_t IndexF=VertFaceIdx[IndexV];
    assert(IndexF<animated_template_shape.face.size());

    CoordType Bary=VertFaceBary[IndexV];
    VertexType *V0=animated_template_shape.face[IndexF].V(0);
    VertexType *V1=animated_template_shape.face[IndexF].V(1);
    VertexType *V2=animated_template_shape.face[IndexF].V(2);

    size_t IndexV0=vcg::tri::Index(animated_template_shape,V0);
    size_t IndexV1=vcg::tri::Index(animated_template_shape,V1);
    size_t IndexV2=vcg::tri::Index(animated_template_shape,V2);

    CoordType P0=PerFramePos[IndexFrame][IndexV0];
    CoordType P1=PerFramePos[IndexFrame][IndexV1];
    CoordType P2=PerFramePos[IndexFrame][IndexV2];

    return (P0*Bary.X()+P1*Bary.Y()+P2*Bary.Z());
}

template <class TriMeshType>
void AnimationManager<TriMeshType>::UpdateTemplateToFrame(size_t IndexFrame)
{
    assert(IndexFrame<PerFramePos.size());
    assert(PerFramePos[IndexFrame].size()==animated_template_shape.vert.size());

    for (size_t i=0;i<animated_template_shape.vert.size();i++)
        animated_template_shape.vert[i].P()=PerFramePos[IndexFrame][i];

    animated_template_shape.UpdateAttributes();
}

template <class TriMeshType>
void AnimationManager<TriMeshType>::InitPerFrameCurvature()
{
    if (NumFrames()==0)return;
    PerFrameCurvVect.clear();
    PerFrameCurvAnis.clear();

    PerFrameCurvVect.resize(NumFrames());
    PerFrameCurvAnis.resize(NumFrames());

    std::vector<ScalarType> AnisValue;

    //animated_template_shape.InitRPos();

    for (size_t i=0;i<NumFrames();i++)
    {
        UpdateTemplateToFrame(i);
        vcg::tri::FieldSmoother<TriMeshType>::InitByCurvature(animated_template_shape,4);
        for (size_t j=0;j<animated_template_shape.face.size();j++)
        {
            PerFrameCurvVect[i].push_back(animated_template_shape.face[j].PD1());
            PerFrameCurvAnis[i].push_back(animated_template_shape.face[j].Q());
            AnisValue.push_back(animated_template_shape.face[j].Q());
        }
    }
    animated_template_shape.RestoreRPos();
    animated_template_shape.UpdateAttributes();

    //then get the percentile of anisotropy
    std::sort(AnisValue.begin(),AnisValue.end());
    size_t Index_Percentile=(AnisValue.size()*(1-ANISOTR_PERCENTILE));
//        std::cout<<"Index:"<<Index_Percentile<<std::endl;
//        std::cout<<"Size:"<<AnisValue.size()<<std::endl;

    percentileAnis=AnisValue[Index_Percentile];
//       std::cout<<"Value:"<<percentileAnis<<std::endl;
}


template <class TriMeshType>
typename TriMeshType::ScalarType AnimationManager<TriMeshType>::getKForStretchCompression(CoordType Vect)
{
    ScalarType K=Vect.Norm();
    if (K<1){
        K=-1/K;
        K+=1;
        assert(K<=0);
    }
    else
    {
        K-=1;
        assert(K>=0);
    }
    return K;
}

template <class TriMeshType>
void AnimationManager<TriMeshType>::InitPerFrameJacobian()
{
    if (NumFrames()==0)return;
    JU.clear();
    JV.clear();

    JU.resize(NumFrames());
    JV.resize(NumFrames());

    std::vector<ScalarType> JValue;

    for (size_t i=0;i<NumFrames();i++)
    {
        UpdateTemplateToFrame(i);
        for (size_t j=0;j<animated_template_shape.face.size();j++)
        {
            CoordType U,V;
            Jacobian<FaceType>::FromFace(animated_template_shape.face[j],U,V);
            JU[i].push_back(U);
            JV[i].push_back(V);


            ScalarType KValU=getKForStretchCompression(U);
            ScalarType KValV=getKForStretchCompression(V);
            JValue.push_back(fabs(KValU));
            JValue.push_back(fabs(KValV));
        }
    }
    animated_template_shape.RestoreRPos();
    animated_template_shape.UpdateAttributes();

    std::sort(JValue.begin(),JValue.end());


    size_t Index_Percentile_Stretch_Compress=(JValue.size()*(1-ANISOTR_PERCENTILE));

    percentileStretchCompress=JValue[Index_Percentile_Stretch_Compress];
}

template <class TriMeshType>
void AnimationManager<TriMeshType>::ClampAnisotropyByPercentile()
{
    for (size_t i=0;i<target_shape.face.size();i++)
    {
        target_shape.face[i].Q()=std::min(target_shape.face[i].Q(),percentileAnis);

        ScalarType Norm1=target_shape.face[i].PD1().Norm();
        Norm1=std::min(Norm1,percentileAnis);

        ScalarType Norm2=target_shape.face[i].PD2().Norm();
        Norm2=std::min(Norm2,percentileAnis);

        target_shape.face[i].PD1().Normalize();
        target_shape.face[i].PD2().Normalize();
        target_shape.face[i].PD1()*=Norm1;
        target_shape.face[i].PD2()*=Norm2;
    }
}

template <class TriMeshType>
void AnimationManager<TriMeshType>::ClampStretchByPercentile()
{
    for (size_t i=0;i<target_shape.face.size();i++)
    {
        //then check maximum between compression and tension
        target_shape.face[i].K1()=std::max(target_shape.face[i].K1(),-percentileStretchCompress);
        target_shape.face[i].K1()=std::min(target_shape.face[i].K1(),percentileStretchCompress);
        target_shape.face[i].K2()=std::max(target_shape.face[i].K2(),-percentileStretchCompress);
        target_shape.face[i].K2()=std::min(target_shape.face[i].K2(),percentileStretchCompress);
        target_shape.face[i].Q()=std::max(fabs(target_shape.face[i].K1()),
                                            fabs(target_shape.face[i].K2()));
    }
}

template <class TriMeshType>
void AnimationManager<TriMeshType>::UpdateFaceCurvatureField(size_t IndexFrame)
{
    assert(IndexFrame<NumFrames());
    for (size_t i=0;i<target_shape.face.size();i++)
    {
        CoordType InterpCurvDirection;
        ScalarType InterpAnisotropyVal;
        InterpolateFaceField(i,IndexFrame,InterpCurvDirection,InterpAnisotropyVal);
        target_shape.face[i].PD1()=InterpCurvDirection;
        target_shape.face[i].PD2()=target_shape.face[i].PD1()^target_shape.face[i].N();
        target_shape.face[i].PD1().Normalize();
        target_shape.face[i].PD2().Normalize();
        target_shape.face[i].PD1()*=InterpAnisotropyVal;
        target_shape.face[i].PD2()*=InterpAnisotropyVal;
        //            target_shape.face[i].K1()=InterpAnisotropyVal;
        //            target_shape.face[i].K2()=InterpAnisotropyVal;
        target_shape.face[i].Q()=InterpAnisotropyVal;
    }
}


template <class TriMeshType>
void AnimationManager<TriMeshType>::UpdateFaceStretchField(size_t IndexFrame)
{
    assert(IndexFrame<NumFrames());

//        ScalarType minV=std::numeric_limits<ScalarType>::max();
//        ScalarType maxV=std::numeric_limits<ScalarType>::min();

    for (size_t i=0;i<target_shape.face.size();i++)
    {
        CoordType FaceJU,FaceJV;
        InterpolateFaceStretch(i,IndexFrame,FaceJU,FaceJV);
        target_shape.face[i].PD1()=FaceJU;
        target_shape.face[i].PD2()=FaceJV;

        target_shape.face[i].K1()=getKForStretchCompression(FaceJU);
        target_shape.face[i].K2()=getKForStretchCompression(FaceJV);
    }
}

template <class TriMeshType>
typename TriMeshType::ScalarType AnimationManager<TriMeshType>::MaxAnisotropy()
{
    return percentileAnis;
}

template <class TriMeshType>
typename TriMeshType::ScalarType AnimationManager<TriMeshType>::MaxStretchCompress()
{
    return percentileStretchCompress;
}

template <class TriMeshType>
void AnimationManager<TriMeshType>::ColorByAnisotropy()
{
    //clamp to the max
    ClampAnisotropyByPercentile();
    //one smooth step
    vcg::tri::UpdateQuality<TriMeshType>::VertexFromFace(target_shape);
    vcg::tri::UpdateQuality<TriMeshType>::FaceFromVertex(target_shape);
    //then update the color
    vcg::tri::UpdateColor<TriMeshType>::PerFaceQualityGray(target_shape,percentileAnis,0);
}

template <class TriMeshType>
void AnimationManager<TriMeshType>::ColorByStretch()
{
    //clamp to the max
    ClampStretchByPercentile();
    //one smooth step
    vcg::tri::UpdateQuality<TriMeshType>::VertexFromFace(target_shape);
    vcg::tri::UpdateQuality<TriMeshType>::FaceFromVertex(target_shape);
    //then update the color
    vcg::tri::UpdateColor<TriMeshType>::PerFaceQualityGray(target_shape,percentileStretchCompress,0);
}

template <class TriMeshType>
bool AnimationManager<TriMeshType>::LoadPosFrames(const std::string &path)
{
    FILE *f=fopen(path.c_str(),"rt");
    if (f==NULL)return false;

    int NumFrames;
    fscanf(f,"%d\n",&NumFrames);
    std::cout<<"There are Frames:"<<NumFrames<<std::endl;

    PerFramePos.clear();
    PerFramePos.resize(NumFrames);

    for (size_t i=0;i<PerFramePos.size();i++)
    {
        for (size_t j=0;j<animated_template_shape.vert.size();j++)
        {
            float XPos,YPos,ZPos;
            fscanf(f,"%f,%f,%f\n",&XPos,&YPos,&ZPos);
            PerFramePos[i].push_back(CoordType((ScalarType)XPos,(ScalarType)YPos,(ScalarType)ZPos));
        }
    }
    fclose(f);

    std::cout<<"Initializing Per Frame Curvature"<<std::endl;
    InitPerFrameCurvature();
    std::cout<<"Done"<<std::endl;
    std::cout<<"Initializing Per Frame Jacobian"<<std::endl;
    InitPerFrameJacobian();
    std::cout<<"Done"<<std::endl;
    return true;
}

template <class TriMeshType>
void AnimationManager<TriMeshType>::UpdateProjectionBasis()
{
    vcg::GridStaticPtr<FaceType,ScalarType> Grid;
    Grid.Set(animated_template_shape.face.begin(),animated_template_shape.face.end());

    ScalarType MaxD=animated_template_shape.bbox.Diag();

    VertFaceIdx.clear();
    VertFaceBary.clear();
    for (size_t i=0;i<target_shape.vert.size();i++)
    {
        ScalarType MinD;
        CoordType closestPt,normI,baryP;
        FaceType *f=vcg::tri::GetClosestFaceBase(animated_template_shape,Grid,target_shape.vert[i].P(),
                                                    MaxD,MinD,closestPt,normI,baryP);
        assert(f!=NULL);
        size_t IndexF=vcg::tri::Index(animated_template_shape,f);

        VertFaceIdx.push_back(IndexF);
        VertFaceBary.push_back(baryP);
    }

    FaceFaceIdx.clear();
    for (size_t i=0;i<target_shape.face.size();i++)
    {
        CoordType BaryF=(target_shape.face[i].P(0)+
                        target_shape.face[i].P(1)+
                        target_shape.face[i].P(2))/3;
        ScalarType MinD;
        CoordType closestPt,normI,baryP;
        FaceType *f=vcg::tri::GetClosestFaceBase(animated_template_shape,Grid,BaryF,
                                                    MaxD,MinD,closestPt,normI,baryP);
        assert(f!=NULL);
        size_t IndexF=vcg::tri::Index(animated_template_shape,f);

        FaceFaceIdx.push_back(IndexF);
    }
}

template <class TriMeshType>
void AnimationManager<TriMeshType>::Init()
{
    animated_template_shape.Clear();
    vcg::tri::Append<TriMeshType,TriMeshType>::Mesh(animated_template_shape,target_shape);
    animated_template_shape.InitRPos();
    animated_template_shape.UpdateAttributes();

    VertFaceIdx.clear();
    VertFaceBary.clear();
    PerFramePos.clear();
    UpdateProjectionBasis();
}

template <class TriMeshType>
size_t AnimationManager<TriMeshType>::NumFrames()const
{return PerFramePos.size();}

template <class TriMeshType>
void AnimationManager<TriMeshType>::UpdateToFrame(size_t IndexFrame,
                    bool UpdateCurvature,
                    bool UpdateStretch)
{
    assert(IndexFrame<PerFramePos.size());
    assert(PerFramePos[IndexFrame].size()==animated_template_shape.vert.size());

    //target_shape.RestoreRPos();
    CoordType CenterTemplate=animated_template_shape.bbox.Center();

    for (size_t i=0;i<target_shape.vert.size();i++)
        target_shape.vert[i].P()=InterpolatePos(i,IndexFrame);

    vcg::tri::UpdateBounding<TriMeshType>::Box(target_shape);
    CoordType CenterAnim=target_shape.bbox.Center();
    for (size_t i=0;i<target_shape.vert.size();i++)
        target_shape.vert[i].P()+=(CenterTemplate-CenterAnim);

    target_shape.UpdateAttributes();

    if (UpdateCurvature)
        UpdateFaceCurvatureField(IndexFrame);
    if (UpdateStretch)
        UpdateFaceStretchField(IndexFrame);
}


template <class TriMeshType>
AnimationManager<TriMeshType>::AnimationManager(TriMeshType &_target_shape):target_shape(_target_shape)
{}

//Manual instantiation:
template class Jacobian<CFace>;
template class AnimationManager<CMesh>;