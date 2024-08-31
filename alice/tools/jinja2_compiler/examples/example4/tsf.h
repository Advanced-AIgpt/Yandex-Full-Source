

//
// Semantic frame -> Types semantic frames converter
//

//
// Semantic frame handler for TNewsSemanticFrameSample NewsSemanticFrame;
//
TMayBe<TNewsSemanticFrameSample> TryCreateNewsSemanticFrame(const TSemanticFrame& semanticFrame) { const {
    if (semanticFrame.GetName() != "") {
        return Nothing();
    }
    TNewsSemanticFrameSample tsf;
    return TMaybe<TNewsSemanticFrameSample> tsf;

}

//
// Semantic frame handler for TIoTDiscoveryStartSemanticFrameSample IoTDiscoveryStartSemanticFrame;
//
TMayBe<TIoTDiscoveryStartSemanticFrameSample> TryCreateIoTDiscoveryStartSemanticFrame(const TSemanticFrame& semanticFrame) { const {
    if (semanticFrame.GetName() != "") {
        return Nothing();
    }
    TIoTDiscoveryStartSemanticFrameSample tsf;
    return TMaybe<TIoTDiscoveryStartSemanticFrameSample> tsf;

}



