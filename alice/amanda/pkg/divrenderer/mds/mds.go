package mds

import (
	"fmt"
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/awserr"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/s3"
	"github.com/aws/aws-sdk-go/service/s3/s3manager"
	"io"
	neturl "net/url"
	"strings"
)

const (
	NotFoundAWSErrorCode  = "NotFound"
	NoSuchKeyAWSErrorCode = "NoSuchKey"
)

type CloudStorage struct {
	sess     *session.Session
	svc      *s3.S3
	uploader *s3manager.Uploader
	bucket   string
}

func New(cfg *aws.Config, bucket string) (s *CloudStorage) {
	s = &CloudStorage{}
	s.sess = session.Must(session.NewSession(cfg))
	s.svc = s3.New(s.sess)
	s.uploader = s3manager.NewUploaderWithClient(s.svc, func(uploader *s3manager.Uploader) {
		// TODO: set up uploader options
	})
	s.bucket = bucket
	_, _ = s.svc.CreateBucket(&s3.CreateBucketInput{
		Bucket: aws.String(bucket),
	})
	return
}

func isAwsNotExist(err error) bool {
	if awsErr, ok := err.(awserr.Error); ok {
		if awsErr.Code() == NotFoundAWSErrorCode || awsErr.Code() == NoSuchKeyAWSErrorCode {
			return true
		}
	}
	return false
}

func (s *CloudStorage) Exists(key string) (bool, error) {
	_, err := s.svc.HeadObject(&s3.HeadObjectInput{
		Bucket: aws.String(s.bucket),
		Key:    aws.String(key),
	})
	if err != nil {
		if isAwsNotExist(err) {
			return false, nil
		}
		return false, fmt.Errorf("failed to check object with key %s: %w", key, err)
	}
	return true, nil
}

func (s *CloudStorage) Share(key string) (url string, err error) {
	//if exists, err := s.Exists(key); !exists || err != nil {
	//	if err != nil {
	//		return "", err
	//	}
	//	err = fmt.Errorf("the specified key doesn't exist: %s", key)
	//	return "", err
	//}
	addr, err := neturl.Parse(aws.StringValue(s.sess.Config.Endpoint))
	if err != nil {
		return "", err
	}
	url = fmt.Sprintf("http://%s.%s/%s", s.bucket, addr.Host, key)
	url = strings.ReplaceAll(url, "mds.", "")
	return url, nil
}

func (s *CloudStorage) Delete(key string) error {
	_, err := s.svc.DeleteObject(&s3.DeleteObjectInput{
		Bucket: aws.String(s.bucket),
		Key:    aws.String(key),
	})
	if err != nil {
		return fmt.Errorf("failed to delete object with key %s: %w", key, err)
	}
	return nil
}

func (s *CloudStorage) Upload(key string, content io.Reader) error {
	u, err := s.uploader.Upload(&s3manager.UploadInput{
		Body:   content,
		Bucket: aws.String(s.bucket),
		Key:    aws.String(key),
	})
	if err != nil {
		return fmt.Errorf("failed to upload object with key %s to bucket %s: %w", key, s.bucket, err)
	}
	fmt.Printf("file successfully uploaded: %s on location %s\n", u.UploadID, u.Location)
	return nil
}
