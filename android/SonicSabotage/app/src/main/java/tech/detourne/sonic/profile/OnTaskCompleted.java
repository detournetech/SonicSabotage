package tech.detourne.sonic.profile;

public interface OnTaskCompleted {
    void onDownloadTaskCompleted(String response);
    void onUploadTaskCompleted(String response);
}